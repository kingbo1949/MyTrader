"""
UTurnBSync_RatioEx 策略（增强版）：
在 UTurnBSync_Ratio 基础上，添加：
1. Ratio Call Spread 到期滚动
2. Ratio Put Spread 接货机制

─── 单元结构 ──────────────────────────────────────────────────────────────────
每个持仓单元 = 正股（核心）+ Ratio Call Spread（可选）+ Ratio Put Spread（可选）
两个 Spread 均围绕同一手正股运行，可同时并存。

─── Call Spread 到期规则 ─────────────────────────────────────────────────────
  S ≥ K2 → 正股被 call away（全部平仓，同时 MTM 平仓活跃 Put Spread）
  S < K2 → 锁定 call options PnL，进入滚动：
    ratio_roll_mode = "immediate" → 当期到期 bar 立刻开新 ratio
    ratio_roll_mode = "signal"   → 等下一个 ratio 信号

─── Put Spread 结构（1:2 Ratio Put Spread）───────────────────────────────────
  买 1 手 K_pl Put（高行权价，接近 ATM，|delta| = put_entry_long）
  卖 2 手 K_ps Put（低行权价，更 OTM，|delta| = put_entry_short）
  net_credit_put = 2*p_ps - p_pl（通常为正）

─── Put Spread 到期规则 ──────────────────────────────────────────────────────
  S ≥ K_pl         → 全部 OTM → 锁定 net_credit，滚动
  K_ps ≤ S < K_pl  → Long Put ITM → 锁定 (K_pl-S)+net_credit，滚动
  S < K_ps         → 接货 → 新单元（entry = S，Options PnL 入 _closed_positions）

接货优先级：按 K_ps 升序（越低越优惠），受 max_positions 限制，
超出容量的其余 put spread 以 MTM 市值平仓。

─── broker 订单设计 ──────────────────────────────────────────────────────────
正股：LONG @ S0（单元开仓）；SHORT @ S0+total_stock_call_pnl（最终平仓）
Put spread 无独立 broker 订单，盈亏全部通过 _closed_positions 入净值曲线。
"""
from datetime import timedelta

import qib_trader.utils.option_pricing as op
from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.uturn_bsync_ratio import UTurnBSync_Ratio
from qib_trader.utils.config_loader import get_iv

# _closed_positions 中属于"中间/辅助"记录的 close_reason，不计入交易指标
_ROLL_REASONS = frozenset({
    'roll_expiry',            # call spread 中间滚仓
    'put_roll_expiry',        # put spread 中间滚仓
    'put_assignment',         # put spread 被接货（新单元），非完整平仓
    'put_assignment_rejected',# put spread 接货失败 MTM 平仓
    'put_cancelled_call',     # call 被 call away 时同步 MTM 平仓 put spread
    'put_cancelled_force',    # force_close 时同步 MTM 平仓 put spread
})


class UTurnBSync_RatioEx(UTurnBSync_Ratio):
    """
    UTurnBSync_Ratio + Call Spread 滚动 + Put Spread 接货。
    """

    # ── Call Spread 滚动参数 ───────────────────────────────────────────────────
    ratio_roll_mode:  str  = "immediate"  # "immediate"=立刻滚；"signal"=等信号
    max_roll_count:   int  = 0            # 0=不限制
    roll_option_days: int  = 0            # 0=复用 option_days
    roll_entry_long:  float = 0.0         # 0=复用 entry_long
    roll_entry_short: float = 0.0         # 0=复用 entry_short

    # ── Put Spread 参数 ────────────────────────────────────────────────────────
    enable_put_spread:   bool  = False
    put_entry_signal:    str   = "bottom"  # "bottom"=随正股；"top"=等 UTurnTop
    put_option_days:     int   = 0         # 0=复用 option_days
    put_entry_long:      float = 0.5       # long put |delta|（K_pl，较高行权价）
    put_entry_short:     float = 0.25      # short put |delta|（K_ps，较低行权价）
    put_roll_mode:       str   = "immediate"
    put_max_roll_count:  int   = 0

    # ── 有效参数（Call Spread）────────────────────────────────────────────────

    def _eff_option_days(self) -> int:
        return self.roll_option_days or self.option_days

    def _eff_entry_long(self) -> float:
        return self.roll_entry_long or self.entry_long

    def _eff_entry_short(self) -> float:
        return self.roll_entry_short or self.entry_short

    # ── 有效参数（Put Spread）─────────────────────────────────────────────────

    def _eff_put_option_days(self) -> int:
        return self.put_option_days or self.option_days

    # ── 初始化单元字典 ────────────────────────────────────────────────────────

    def _make_unit(self, bar: BarData) -> dict:
        """构建带 put spread 字段的持仓单元。"""
        return {
            # 正股
            'entry_dt':         bar.datetime,
            'entry_underlying': bar.close_price,
            'roll_count':       0,
            'locked_pnl':       0.0,   # call spread 历史 options PnL
            # Call Spread
            'ratio_opened':  False, 'ratio_open_dt': None,
            'k1': None, 'k2': None, 'p1': None, 'p2': None,
            'net_debit': None, 'iv1': None, 'iv2': None, 'expiry': None,
            # Put Spread
            'put_opened':    False, 'put_open_dt': None,
            'p_k_pl': None, 'p_k_ps': None,
            'p_p_pl': None, 'p_p_ps': None,
            'p_iv_pl': None, 'p_iv_ps': None,
            'p_net_credit': None, 'p_expiry': None,
            'put_roll_count': 0,
            'put_locked_pnl': 0.0,  # put spread 历史 options PnL（已入记录）
        }

    # ─────────────────────────────────────────────────────────────────────────
    # CALL SPREAD 方法（覆盖 / 继承自父类）
    # ─────────────────────────────────────────────────────────────────────────

    def _open_ratio_spread_ex(self, p: dict, bar: BarData) -> None:
        """用滚仓参数开 Ratio Call Spread。"""
        S      = bar.close_price
        days   = self._eff_option_days()
        expiry = self._next_expiry(bar.datetime, days)
        T      = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        dL     = self._eff_entry_long()
        dS     = self._eff_entry_short()
        iv1    = get_iv(self.codeId, 'call', dL)
        iv2    = get_iv(self.codeId, 'call', dS)
        k1     = self._call_strike_by_delta(S, T, dL, iv1)
        k2     = self._call_strike_by_delta(S, T, dS, iv2)
        p1     = op.call_price(S, k1, T, self.risk_free_rate, iv1, self.codeId)
        p2     = op.call_price(S, k2, T, self.risk_free_rate, iv2, self.codeId)
        p.update({'k1': k1, 'k2': k2, 'p1': p1, 'p2': p2,
                  'net_debit': p1 - 2.0 * p2, 'iv1': iv1, 'iv2': iv2,
                  'expiry': expiry, 'ratio_open_dt': bar.datetime,
                  'ratio_opened': True})
        self.logger.info(
            f"[{bar.datetime}] call_ratio K1={k1} K2={k2} S={S:.2f} "
            f"nd={p['net_debit']:.4f} roll#{p['roll_count']} exp={expiry.date()}"
        )

    def _period_options_pnl(self, S: float, p: dict) -> float:
        """本期 call spread options 损益（不含正股）。"""
        k1, k2, nd = p['k1'], p['k2'], p['net_debit']
        if S < k1:
            return -nd
        elif S < k2:
            return (S - k1) - nd
        else:
            return (k2 - k1) - nd

    def _roll_options(self, p: dict, bar: BarData) -> None:
        """S < K2 到期：锁定 call options PnL，清空 call 字段，视模式立刻或等信号重开。"""
        S        = bar.close_price
        opts_pnl = self._period_options_pnl(S, p)
        self._record_roll_expiry(p, bar, opts_pnl)
        p['locked_pnl']  += opts_pnl
        p['roll_count']   += 1
        for f in ('k1', 'k2', 'p1', 'p2', 'net_debit', 'iv1', 'iv2',
                  'expiry', 'ratio_open_dt'):
            p[f] = None
        p['ratio_opened'] = False
        self.logger.info(
            f"[{bar.datetime}] call滚仓#{p['roll_count']} S={S:.2f} "
            f"opts={opts_pnl:.2f} locked={p['locked_pnl']:.2f}"
        )
        if self.ratio_roll_mode == "immediate":
            self._open_ratio_spread_ex(p, bar)

    # ─────────────────────────────────────────────────────────────────────────
    # PUT SPREAD 方法（新增）
    # ─────────────────────────────────────────────────────────────────────────

    def _put_strike_by_delta(self, S: float, T: float, delta_abs: float, iv: float) -> float:
        return op.put_strike_by_delta(
            S, T, delta_abs, iv, self.risk_free_rate, self.codeId, self.strike_round)

    def _curr_put_prices(self, bar: BarData, p: dict) -> tuple[float, float]:
        """返回 (pl_curr, ps_curr) — long put K_pl 和 short put K_ps 的当前价格。"""
        T    = self._rem_t(bar.datetime, p['p_expiry'])
        S    = bar.close_price
        pl   = op.put_price(S, p['p_k_pl'], T, self.risk_free_rate, p['p_iv_pl'], self.codeId)
        ps   = op.put_price(S, p['p_k_ps'], T, self.risk_free_rate, p['p_iv_ps'], self.codeId)
        return pl, ps

    def _open_put_spread(self, p: dict, bar: BarData) -> None:
        """为持仓单元开仓 1:2 Ratio Put Spread。"""
        S      = bar.close_price
        days   = self._eff_put_option_days()
        expiry = self._next_expiry(bar.datetime, days)
        T      = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        dL     = self.put_entry_long    # long put |delta|（K_pl，较高行权价）
        dS     = self.put_entry_short   # short put |delta|（K_ps，较低行权价）
        iv_pl  = get_iv(self.codeId, 'put', dL)
        iv_ps  = get_iv(self.codeId, 'put', dS)
        k_pl   = self._put_strike_by_delta(S, T, dL, iv_pl)
        k_ps   = self._put_strike_by_delta(S, T, dS, iv_ps)
        p_pl   = op.put_price(S, k_pl, T, self.risk_free_rate, iv_pl, self.codeId)
        p_ps   = op.put_price(S, k_ps, T, self.risk_free_rate, iv_ps, self.codeId)
        net_c  = 2.0 * p_ps - p_pl
        p.update({'put_opened': True, 'put_open_dt': bar.datetime,
                  'p_k_pl': k_pl, 'p_k_ps': k_ps,
                  'p_p_pl': p_pl, 'p_p_ps': p_ps,
                  'p_iv_pl': iv_pl, 'p_iv_ps': iv_ps,
                  'p_net_credit': net_c, 'p_expiry': expiry})
        self.logger.info(
            f"[{bar.datetime}] put_spread K_pl={k_pl} K_ps={k_ps} S={S:.2f} "
            f"net_c={net_c:.4f} roll#{p['put_roll_count']} exp={expiry.date()}"
        )

    def _period_put_pnl(self, S: float, p: dict) -> float:
        """本期 put spread options 损益：
        S ≥ K_pl     → net_credit
        K_ps≤S<K_pl  → (K_pl-S) + net_credit
        S < K_ps     → net_credit + S + K_pl - 2*K_ps  （接货时的期权盈亏）
        """
        k_pl, k_ps, nc = p['p_k_pl'], p['p_k_ps'], p['p_net_credit']
        if S >= k_pl:
            return nc
        elif S >= k_ps:
            return (k_pl - S) + nc
        else:
            return nc + S + k_pl - 2.0 * k_ps

    def _roll_put_spread(self, p: dict, bar: BarData, opts_pnl: float) -> None:
        """锁定 put spread 本期 options PnL，清空 put 字段，视模式立刻或等信号重开。"""
        self._record_put_expiry(p, bar, opts_pnl, 'put_roll_expiry')
        p['put_locked_pnl']  += opts_pnl
        p['put_roll_count']  += 1
        self._clear_put_spread(p)
        self.logger.info(
            f"[{bar.datetime}] put滚仓#{p['put_roll_count']} "
            f"opts={opts_pnl:.2f} locked={p['put_locked_pnl']:.2f}"
        )
        if self.put_roll_mode == "immediate":
            self._open_put_spread(p, bar)

    def _execute_put_assignment(self, p: dict, bar: BarData) -> None:
        """接货：创建新单元（LONG @ S），put options PnL 记入 _closed_positions。"""
        S      = bar.close_price
        opts   = self._period_put_pnl(S, p)          # 通常为负（期权换股代价）
        self._record_put_expiry(p, bar, opts, 'put_assignment')
        new_p  = self._make_unit(bar)                 # entry_underlying = S
        self._positions.append(new_p)
        self._send_order(Direction.LONG, S, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] put接货 K_pl={p['p_k_pl']} K_ps={p['p_k_ps']} S={S:.2f} "
            f"opts={opts:.2f} eff_cost={(2*p['p_k_ps']-p['p_k_pl']-p['p_net_credit']):.2f}"
        )
        # 新单元按信号策略立刻或延迟挂载 spread
        if self.ratio_entry_signal == "bottom":
            self._open_ratio_spread_ex(new_p, bar)
        if self.enable_put_spread and self.put_entry_signal == "bottom":
            self._open_put_spread(new_p, bar)

    def _mtm_close_put_spread(self, p: dict, bar: BarData, reason: str) -> None:
        """MTM 平仓 put spread（正股继续或同时关闭），本期 options PnL 入记录。"""
        pl, ps = self._curr_put_prices(bar, p)
        opts   = (pl - p['p_p_pl']) - 2.0 * (ps - p['p_p_ps'])
        self._record_put_expiry(p, bar, opts, reason)
        self.logger.info(
            f"[{bar.datetime}] {reason} K_pl={p['p_k_pl']} K_ps={p['p_k_ps']} "
            f"S={bar.close_price:.2f} put_opts={opts:.2f}"
        )

    def _clear_put_spread(self, p: dict) -> None:
        """重置 put spread 字段。"""
        for f in ('p_k_pl', 'p_k_ps', 'p_p_pl', 'p_p_ps',
                  'p_iv_pl', 'p_iv_ps', 'p_net_credit',
                  'p_expiry', 'put_open_dt'):
            p[f] = None
        p['put_opened'] = False

    def _check_put_expiry(self, bar: BarData) -> None:
        """处理 put spread 到期：滚仓 or 接货 or 容量满则 MTM 平仓。"""
        S = bar.close_price
        assignments = []
        for p in self._positions:
            if not p['put_opened'] or bar.datetime < p['p_expiry']:
                continue
            opts = self._period_put_pnl(S, p)
            if S >= p['p_k_ps']:
                # OTM 或部分 ITM → 滚仓
                self._roll_put_spread(p, bar, opts)
            else:
                # S < K_ps → 接货
                assignments.append(p)

        # 按 K_ps 升序排（行权价更低 = 更优惠 = 优先接货）
        assignments.sort(key=lambda pos: pos['p_k_ps'])
        available = max(0, self.max_positions - len(self._positions))
        for i, p in enumerate(assignments):
            if i < available:
                self._execute_put_assignment(p, bar)
            else:
                self._mtm_close_put_spread(p, bar, 'put_assignment_rejected')
            self._clear_put_spread(p)

    def _check_call_expiry(self, bar: BarData) -> None:
        """处理 call spread 到期：滚仓 or 全部平仓（同时 MTM 平仓活跃 put spread）。"""
        remaining = []
        for p in self._positions:
            if not p['ratio_opened'] or bar.datetime < p['expiry']:
                remaining.append(p)
                continue
            S             = bar.close_price
            called_away   = S >= p['k2']
            max_hit       = self.max_roll_count > 0 and p['roll_count'] >= self.max_roll_count
            if called_away or max_hit:
                reason = 'max_roll_exceeded' if (max_hit and not called_away) else 'expiry'
                self._final_close_on_expiry(p, bar, reason)
                # 注意：_final_close_on_expiry 已包含 put spread MTM 处理
            else:
                self._roll_options(p, bar)
                remaining.append(p)
        self._positions = remaining

    # ─────────────────────────────────────────────────────────────────────────
    # 覆盖父类：全部平仓（call 接货 or max_roll）
    # ─────────────────────────────────────────────────────────────────────────

    def _final_close_on_expiry(self, p: dict, bar: BarData,
                               close_reason: str = 'expiry') -> None:
        """全平：先 MTM 平仓活跃 put spread，再平仓正股+call spread。"""
        if p['put_opened']:
            self._mtm_close_put_spread(p, bar, 'put_cancelled_call')
            self._clear_put_spread(p)
        S, S0 = bar.close_price, p['entry_underlying']
        if p['ratio_opened']:
            opts_pnl    = self._period_options_pnl(S, p)
            stock_close = p['k2'] if S >= p['k2'] else S
        else:
            opts_pnl, stock_close = 0.0, S
        stock_pnl  = stock_close - S0
        total_pnl  = stock_pnl + p['locked_pnl'] + opts_pnl
        record_pnl = stock_pnl + opts_pnl
        self._send_order(Direction.SHORT, S0 + total_pnl, 1, "MKT")
        self._record_final_close(p, bar, close_reason, record_pnl)
        self.logger.info(
            f"[{bar.datetime}] 终止({close_reason}) S={S:.2f} "
            f"locked={p['locked_pnl']:.2f} opts={opts_pnl:.2f} total={total_pnl:.2f}"
        )

    def _close_position(self, p: dict, bar: BarData, close_reason: str) -> None:
        """MTM 提前平仓（force_close / stop）：先 MTM 平仓 put spread，再平仓正股+call。"""
        if p['put_opened']:
            self._mtm_close_put_spread(p, bar, 'put_cancelled_force')
            self._clear_put_spread(p)
        S, S0 = bar.close_price, p['entry_underlying']
        if p['ratio_opened']:
            c1, c2   = self._curr_call_prices(bar, p)
            opts_pnl = (c1 - p['p1']) - 2.0 * (c2 - p['p2'])
        else:
            opts_pnl = 0.0
        stock_pnl  = S - S0
        total_pnl  = stock_pnl + p['locked_pnl'] + opts_pnl
        record_pnl = stock_pnl + opts_pnl
        self._send_order(Direction.SHORT, S0 + total_pnl, 1, "MKT")
        self._record_final_close(p, bar, close_reason, record_pnl)
        self.logger.info(
            f"[{bar.datetime}] {close_reason} S={S:.2f} "
            f"locked={p['locked_pnl']:.2f} opts_mtm={opts_pnl:.2f} total={total_pnl:.2f}"
        )

    # ─────────────────────────────────────────────────────────────────────────
    # 覆盖父类：盯市（含 put spread）
    # ─────────────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        if not self._positions:
            return 0.0
        S, total = bar.close_price, 0.0
        for p in self._positions:
            stock_mtm = S - p['entry_underlying']
            call_mtm  = 0.0
            if p['ratio_opened']:
                c1, c2   = self._curr_call_prices(bar, p)
                call_mtm = (c1 - p['p1']) - 2.0 * (c2 - p['p2'])
            put_mtm = 0.0
            if p['put_opened']:
                pl, ps  = self._curr_put_prices(bar, p)
                put_mtm = (pl - p['p_p_pl']) - 2.0 * (ps - p['p_p_ps'])
            total += stock_mtm + p['locked_pnl'] + call_mtm + p['put_locked_pnl'] + put_mtm
        return total

    # ─────────────────────────────────────────────────────────────────────────
    # 开仓（覆盖父类：添加 put spread 字段 + put_entry_signal 处理）
    # ─────────────────────────────────────────────────────────────────────────

    def _check_entry(self, bar: BarData) -> None:
        if not self._entry_signal(bar):
            return
        self._last_entry_dt = bar.datetime
        p = self._make_unit(bar)
        self._positions.append(p)
        self._send_order(Direction.LONG, bar.close_price, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓 S={bar.close_price:.2f} "
            f"call_sig={self.ratio_entry_signal}"
        )
        if self.ratio_entry_signal == "bottom":
            self._open_ratio_spread_ex(p, bar)
        if self.enable_put_spread and self.put_entry_signal == "bottom":
            self._open_put_spread(p, bar)

    # ─────────────────────────────────────────────────────────────────────────
    # 信号驱动的 spread 开仓（top / signal 模式）
    # ─────────────────────────────────────────────────────────────────────────

    def _check_roll_signal_bottom(self, bar: BarData) -> None:
        """call spread 滚仓后等 bottom 信号重开（ratio_roll_mode=signal + bottom）。"""
        if self.ratio_roll_mode != "signal" or self.ratio_entry_signal != "bottom":
            return
        if bar.datetime not in self._sync_timestamps:
            return
        for p in self._positions:
            if not p['ratio_opened'] and p['roll_count'] > 0:
                self._open_ratio_spread_ex(p, bar)

    def _check_put_open_top(self, bar: BarData) -> None:
        """put_entry_signal=top：遇到 UTurnTop 信号时，为未开 put spread 的持仓开仓。"""
        if not self.enable_put_spread or self.put_entry_signal != "top":
            return
        if bar.datetime not in self._top_timestamps:
            return
        for p in self._positions:
            if not p['put_opened']:
                self._open_put_spread(p, bar)

    def _check_put_roll_signal_bottom(self, bar: BarData) -> None:
        """put spread 滚仓后等 bottom 信号重开（put_roll_mode=signal + put_entry_signal=bottom）。"""
        if (not self.enable_put_spread
                or self.put_roll_mode != "signal"
                or self.put_entry_signal != "bottom"):
            return
        if bar.datetime not in self._sync_timestamps:
            return
        for p in self._positions:
            if not p['put_opened'] and p['put_roll_count'] > 0:
                self._open_put_spread(p, bar)

    # ─────────────────────────────────────────────────────────────────────────
    # 记录方法
    # ─────────────────────────────────────────────────────────────────────────

    def _record_roll_expiry(self, p: dict, bar: BarData, opts_pnl: float) -> None:
        """记录 call spread 中间滚仓（period options PnL）。"""
        self._closed_positions.append({
            'entry_dt':         p['entry_dt'],
            'entry_underlying': p['entry_underlying'],
            'ratio_open_dt':    p.get('ratio_open_dt'),
            'settlement_dt':    bar.datetime,
            'close_reason':     'roll_expiry',
            'ratio_opened':     True,
            'roll_idx':         p['roll_count'],
            'k1':               p['k1'], 'k2': p['k2'],
            'p1':               p['p1'], 'p2': p['p2'],
            'net_debit':        p['net_debit'],
            'final_pnl':        opts_pnl,
            'notional':         p['k2'] - p['k1'],
        })

    def _record_put_expiry(self, p: dict, bar: BarData,
                           opts_pnl: float, reason: str) -> None:
        """记录 put spread 滚仓、接货、MTM 平仓等辅助事件。"""
        self._closed_positions.append({
            'entry_dt':         p['entry_dt'],
            'entry_underlying': p['entry_underlying'],
            'put_open_dt':      p.get('put_open_dt'),
            'settlement_dt':    bar.datetime,
            'close_reason':     reason,
            'put_roll_idx':     p['put_roll_count'],
            'p_k_pl':           p['p_k_pl'],
            'p_k_ps':           p['p_k_ps'],
            'p_net_credit':     p['p_net_credit'],
            'final_pnl':        opts_pnl,
            'notional':         (p['p_k_pl'] - p['p_k_ps']) if p['p_k_pl'] else 0.0,
        })

    def _record_final_close(self, p: dict, bar: BarData,
                            close_reason: str, total_pnl: float) -> None:
        """最终平仓：记录正股 + call spread 合并盈亏（不含已单独记录的 put 和 roll）。"""
        self._closed_positions.append({
            'entry_dt':         p['entry_dt'],
            'entry_underlying': p['entry_underlying'],
            'ratio_open_dt':    p.get('ratio_open_dt'),
            'settlement_dt':    bar.datetime,
            'close_reason':     close_reason,
            'ratio_opened':     p['ratio_opened'],
            'roll_count':       p['roll_count'],
            'locked_pnl':       p['locked_pnl'],
            'k1':               p.get('k1'), 'k2': p.get('k2'),
            'p1':               p.get('p1'), 'p2': p.get('p2'),
            'net_debit':        p.get('net_debit', 0.0),
            'final_pnl':        total_pnl,
            'notional':         (p['k2'] - p['k1']) if p.get('k2') else 0.0,
        })

    # ─────────────────────────────────────────────────────────────────────────
    # 覆盖父类：统一 expiry 入口
    # ─────────────────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        """先处理 call 到期（可能全平单元），再处理 put 到期（接货/滚仓）。"""
        self._check_call_expiry(bar)
        self._check_put_expiry(bar)

    # ─────────────────────────────────────────────────────────────────────────
    # 主逻辑
    # ─────────────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_ratio_open(bar)          # call top 模式新仓/滚仓信号
        self._check_roll_signal_bottom(bar)  # call signal+bottom 滚仓信号
        self._check_put_open_top(bar)        # put top 模式开仓信号
        self._check_put_roll_signal_bottom(bar)  # put signal+bottom 滚仓信号
        self._check_expiry(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
