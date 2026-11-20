"""
UTurnBSync_CoveredCallEx 策略：
  开仓：与 UTurnBSync_CoveredCall 相同（买正股 + 卖 OTM Call）。
  到期 S ≥ K → 行权，正股在 K 卖出，lifecycle 结束。
  到期 S < K → 收取权利金，保留正股，滚仓开新 Call（roll 参数独立）。

  early/late profit 基于总持仓盈亏（不只是 call 衰减）：
    total_pnl  = (S - S0) + prev_premiums + (P_curr - curr_call)
    max_profit = (K_curr - S0) + prev_premiums + P_curr
    early: total_pnl ≥ early_profit_ratio × max_profit  AND  elapsed < early_time_ratio
    late:  total_pnl ≥ late_profit_ratio  × max_profit  AND  elapsed ≥ late_time_ratio
    （触发时正股 + 当前 call 一起平）

  合成订单（FIFO 配对）：
    LONG  @ S0 - P_initial
    SHORT @ stock_exit - P_initial + prev_premiums + P_curr - call_close
    → pnl = SHORT - LONG = total_pnl ✓
"""
from datetime import timedelta, datetime, time as dtime

from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies._cc_dip_mixin import CoveredCallDipMixin
from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps, _load_bars
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing


class UTurnBSync_CoveredCallEx(CoveredCallDipMixin, Strategy):
    """Covered Call + 自动滚仓：call 虚值到期时保留正股并开新 call，直到行权或触发止盈/止损。"""

    options_mode:    bool  = True
    initial_capital: float = 100_000.0

    sync_signals:       list  = []
    start_time:         str   = "2024-01-01 00:00:00"
    end_time:           str   = "2026-06-01 00:00:00"
    uturn_btm_level:    int   = 1
    multi_uturn_window: int   = 50
    min_dif_abs:        float = 0.0

    # ── 初始 call 参数 ─────────────────────────────────────────────────────────
    entry_short:    float = 0.3
    option_days:    int   = 14
    expiry_weekday: int   = -1

    # ── 滚仓 call 参数（独立）──────────────────────────────────────────────────
    roll_entry_short:    float = 0.2
    roll_option_days:    int   = 14
    roll_expiry_weekday: int   = -1
    max_roll_count:      int   = 0    # 0=不限制；超过后强制平仓

    # ── 共用平仓参数（初始与滚仓相同）─────────────────────────────────────────
    risk_free_rate:     float = 0.04
    strike_round:       float = 100.0
    cooldown_hours:     float = 1.0
    max_positions:      int   = 3
    hard_stop_points:   float = 0.0
    early_profit_ratio: float = 0.0
    early_time_ratio:   float = 0.0
    late_profit_ratio:  float = 0.0
    late_time_ratio:    float = 0.0
    stop_loss_ratio:    float = 0.0

    # ── 下跌回调展期 ──────────────────────────────────────────────────────────
    # UTurnBtm 出现时买回 short call，等 UTurnTop 后再卖出，完成一次展期。
    # 两个信号均可选"同步模式"（所有 sync_signals 同时触发）或"单品种模式"。
    dip_close_level:    int  = 0     # UTurnBtm 等级（0=不启用此功能）
                                     # 1=SimpleUBtm 2=DbSimpleBtm 3=MixDbSimpleBtm
    dip_close_sync:     bool = True   # True=同步信号；False=使用 dip_close_code_id 单品种
    dip_close_code_id:  str  = ""    # 单品种模式下的品种（空串=使用交易标的本身）
    dip_close_window:   int  = 50    # Level 2/3 需要的历史 bar 窗口大小

    dip_reopen_level:   int  = 0     # UTurnTop 等级（0=平仓后立即展期，不等信号）
                                     # 1=SimpleUTop 2=DbSimpleTop 3=MixDbSimpleTop
    dip_reopen_sync:    bool = True   # True=同步信号；False=使用 dip_reopen_code_id 单品种
    dip_reopen_code_id: str  = ""    # 单品种模式下的品种（空串=使用交易标的本身）
    dip_reopen_window:  int  = 50    # Level 2/3 需要的历史 bar 窗口大小
    dip_roll_option_days: int = 0   # 滚动态 dip 重建 call 的到期天数（0=使用 roll_option_days）

    # ── 生命周期 ──────────────────────────────────────────────────────────────

    def on_init(self) -> None:
        self._positions: list[dict] = []
        self._closed_positions: list[dict] = []
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._sync_timestamps: set    = set()
        self._dip_close_timestamps: set  = set()
        self._dip_reopen_timestamps: set = set()

    def on_start(self) -> None:
        self.active = True
        if not self.sync_signals:
            self.logger.warning("sync_signals 为空，策略永远不会开仓")
            return
        self._sync_timestamps = _build_sync_timestamps(
            self.sync_signals, self.interval.value,
            self.start_time, self.end_time,
            level=self.uturn_btm_level, window=self.multi_uturn_window,
        )
        if self.dip_close_level > 0:
            self._dip_close_timestamps = self._build_btm_timestamps(
                self.dip_close_sync, self.dip_close_code_id,
                self.dip_close_level, self.dip_close_window)
            self.logger.info(
                f"下跌平call时间戳 {len(self._dip_close_timestamps)} 个"
                f"（等级{self.dip_close_level} sync={self.dip_close_sync}）"
            )
            if self.dip_reopen_level > 0:
                self._dip_reopen_timestamps = self._build_top_timestamps(
                    self.dip_reopen_sync, self.dip_reopen_code_id,
                    self.dip_reopen_level, self.dip_reopen_window)
                self.logger.info(
                    f"反弹开call时间戳 {len(self._dip_reopen_timestamps)} 个"
                    f"（等级{self.dip_reopen_level} sync={self.dip_reopen_sync}）"
                )

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for p in list(self._positions):
            self._close_all(p, self._last_bar, 'force_close')
        self._positions.clear()
        # 清仓后重置 MTM 为 0，避免 equity 曲线在最后一 bar 出现双重计数
        # （on_stop 在 logic() 之后执行，_mtm_pnl_series[last_bar] 此前已记录浮动盈亏，
        #  而 force_close 又在同一 bar 结算实现盈亏，二者相加会造成重复）
        self._record_mtm_pnl(self._last_bar)

    # ── 工具 ──────────────────────────────────────────────────────────────────

    def _next_expiry(self, bar_dt: datetime, min_days: int, weekday: int) -> datetime:
        target = bar_dt + timedelta(days=min_days)
        if weekday == -1 or min_days == 1:
            extra = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        else:
            days_ahead  = (weekday - target.weekday()) % 7
            expiry_date = (target + timedelta(days=days_ahead)).date()
        return datetime.combine(expiry_date, dtime(16, 0))

    def _rem_t(self, bar_dt: datetime, expiry: datetime) -> float:
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    def _calc_max_profit(self, p: dict) -> float:
        """(K_curr - S0) + prev_premiums + P_curr。"""
        return (p['call_k'] - p['S0']) + p['prev_premiums'] + p['call_prem']

    def _calc_total_pnl(self, bar: BarData, p: dict, S: float = None):
        """返回 (total_pnl, curr_call_price)。S 为 None 时使用 bar.close_price。"""
        if S is None:
            S = bar.close_price
        T         = self._rem_t(bar.datetime, p['call_expiry'])
        curr_call = option_pricing.call_price(S, p['call_k'], T,
                                              self.risk_free_rate, p['call_iv'], self.codeId)
        pnl = (S - p['S0']) + p['prev_premiums'] + (p['call_prem'] - curr_call)
        return pnl, curr_call

    def _elapsed_ratio(self, bar: BarData, p: dict) -> float:
        total   = (p['call_expiry'] - p['call_entry_dt']).total_seconds()
        elapsed = (bar.datetime - p['call_entry_dt']).total_seconds()
        return elapsed / total if total > 0 else 1.0

    # ── 开新 call ─────────────────────────────────────────────────────────────

    def _make_call_fields(self, bar: BarData, S: float,
                          delta: float, opt_days: int, exp_weekday: int) -> dict:
        expiry = self._next_expiry(bar.datetime, opt_days, exp_weekday)
        T      = self._rem_t(bar.datetime, expiry)
        iv     = get_iv(self.codeId, 'call', delta)
        k      = option_pricing.call_strike_by_delta(
            S, T, delta, iv, self.risk_free_rate, self.codeId, self.strike_round)
        prem   = option_pricing.call_price(S, k, T, self.risk_free_rate, iv, self.codeId)
        return {'call_k': k, 'call_prem': prem, 'call_iv': iv,
                'call_expiry': expiry, 'call_entry_dt': bar.datetime}

    # ── 平仓核心 ──────────────────────────────────────────────────────────────

    def _cc_finalize(self, p: dict, bar: BarData, close_reason: str,
                  stock_exit: float, call_close: float) -> None:
        """记录合成 SHORT 订单并存档，不修改 _positions 列表。"""
        total_pnl   = (stock_exit - p['S0']) + p['prev_premiums'] + (p['call_prem'] - call_close)
        short_price = stock_exit - p['P_initial'] + p['prev_premiums'] + p['call_prem'] - call_close
        self._send_order(Direction.SHORT, short_price, 1, "MKT")
        self._closed_positions.append({
            'entry_dt':      p['entry_dt'],   'settlement_dt': bar.datetime,
            'S0':            p['S0'],          'stock_exit':    stock_exit,
            'call_k':        p['call_k'],      'call_prem':     p['call_prem'],
            'call_close':    call_close,       'prev_premiums': p['prev_premiums'],
            'P_initial':     p['P_initial'],   'roll_count':    p['roll_count'],
            'close_reason':  close_reason,     'final_pnl':     total_pnl,
        })
        self.logger.info(
            f"[{bar.datetime}] 平仓 reason={close_reason} rolls={p['roll_count']} "
            f"S0={p['S0']:.2f} exit={stock_exit:.2f} "
            f"prev_prem={p['prev_premiums']:.4f} final_pnl={total_pnl:.4f}"
        )

    def _close_all(self, p: dict, bar: BarData, close_reason: str,
                   stock_price: float = None) -> None:
        """按当前市价平仓正股 + 当前 call（dip_waiting 时无 call，call_close=0）。"""
        S = stock_price if stock_price is not None else bar.close_price
        if p.get('dip_waiting', False):
            self._cc_finalize(p, bar, close_reason, S, 0.0)
        else:
            _, curr_call = self._calc_total_pnl(bar, p, S)
            self._cc_finalize(p, bar, close_reason, S, curr_call)

    # ── 到期结算（含滚仓逻辑）────────────────────────────────────────────────

    def _settle_expiry(self, p: dict, bar: BarData) -> bool:
        """
        到期结算。
        返回 True → 继续持仓（滚仓），position 保留。
        返回 False → lifecycle 结束，position 应从列表移除。
        """
        S, K = bar.close_price, p['call_k']
        if S >= K:
            self._cc_finalize(p, bar, 'expiry_assignment', K, 0.0)
            return False
        # call 虚值作废 → 锁定权利金，滚仓
        p['prev_premiums'] += p['call_prem']
        p['roll_count']    += 1
        if self.max_roll_count > 0 and p['roll_count'] > self.max_roll_count:
            self._cc_finalize(p, bar, 'max_roll_exceeded', S, 0.0)
            return False
        p.update(self._make_call_fields(bar, S, self.roll_entry_short,
                                        self.roll_option_days, self.roll_expiry_weekday))
        self.logger.info(
            f"[{bar.datetime}] 滚仓#{p['roll_count']} "
            f"K={p['call_k']} prem={p['call_prem']:.4f} "
            f"prev={p['prev_premiums']:.4f} expiry={p['call_expiry'].date()}"
        )
        return True

    # ── 各项检查 ──────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in list(self._positions):
            if not p.get('dip_waiting', False) and bar.datetime >= p['call_expiry']:
                if self._settle_expiry(p, bar):   # True = 保留
                    remaining.append(p)
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_hard_stop(self, bar: BarData) -> None:
        if self.hard_stop_points <= 0:
            return
        remaining = []
        for p in self._positions:
            stop_price = p['S0'] - self.hard_stop_points
            if bar.low_price < stop_price:
                self._close_all(p, bar, 'hard_stop', stop_price)
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_stop_loss(self, bar: BarData) -> None:
        if self.stop_loss_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p.get('dip_waiting', False):       # 等待态无 call，跳过
                remaining.append(p)
                continue
            max_p  = self._calc_max_profit(p)
            pnl, _ = self._calc_total_pnl(bar, p)
            if max_p > 0 and pnl < -self.stop_loss_ratio * max_p:
                self._close_all(p, bar, 'stop_loss')
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_early_profit(self, bar: BarData) -> None:
        if self.early_profit_ratio <= 0 or self.early_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p.get('dip_waiting', False):       # 等待态无 call，跳过
                remaining.append(p)
                continue
            max_p  = self._calc_max_profit(p)
            pnl, _ = self._calc_total_pnl(bar, p)
            if (max_p > 0
                    and self._elapsed_ratio(bar, p) < self.early_time_ratio
                    and pnl >= self.early_profit_ratio * max_p):
                self._close_all(p, bar, 'early_profit')
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_late_profit(self, bar: BarData) -> None:
        if self.late_profit_ratio <= 0 or self.late_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p.get('dip_waiting', False):       # 等待态无 call，跳过
                remaining.append(p)
                continue
            max_p  = self._calc_max_profit(p)
            pnl, _ = self._calc_total_pnl(bar, p)
            if (max_p > 0
                    and self._elapsed_ratio(bar, p) >= self.late_time_ratio
                    and pnl >= self.late_profit_ratio * max_p):
                self._close_all(p, bar, 'late_profit')
            else:
                remaining.append(p)
        self._positions = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        if not self._positions:
            return 0.0
        S, total = bar.close_price, 0.0
        for p in self._positions:
            if p.get('dip_waiting', False):
                # 等待态：无 short call，MTM = 正股浮盈 + 已累计权利金
                total += (S - p['S0']) + p['prev_premiums']
            else:
                total += self._calc_total_pnl(bar, p)[0]
        return total

    # ── 开仓信号 ──────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if bar.datetime not in self._sync_timestamps:
            return False
        if self.min_dif_abs > 0 and abs(bar.dif) < self.min_dif_abs:
            return False
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        return not (self.max_positions > 0 and len(self._positions) >= self.max_positions)

    def _check_entry(self, bar: BarData) -> None:
        if not self._entry_signal(bar):
            return
        S         = bar.close_price
        call_flds = self._make_call_fields(bar, S, self.entry_short,
                                           self.option_days, self.expiry_weekday)
        P_initial = call_flds['call_prem']
        p = {'entry_dt': bar.datetime, 'S0': S, 'P_initial': P_initial,
              'prev_premiums': 0.0, 'roll_count': 0, **call_flds}
        self._positions.append(p)
        self._last_entry_dt = bar.datetime
        self._send_order(Direction.LONG, S - P_initial, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓 S={S:.2f} K={p['call_k']} "
            f"prem={P_initial:.4f} net_cost={S - P_initial:.2f} expiry={p['call_expiry'].date()}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_expiry(bar)           # 到期结算（跳过 dip_waiting）
        self._check_dip_close(bar)        # UTurnBtm → 买回 short call
        self._check_dip_reopen(bar)       # UTurnTop → 重新卖出 short call
        self._check_hard_stop(bar)
        self._check_stop_loss(bar)        # 跳过 dip_waiting
        self._check_early_profit(bar)     # 跳过 dip_waiting
        self._check_late_profit(bar)      # 跳过 dip_waiting
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
