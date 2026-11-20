"""
UTurnBSync_DebitCallSpreadEx 策略（两阶段自动转仓）

Phase 1 — Debit Call Spread（Bull Call Spread）：
  开仓：UTurnBottom 同步信号 → 买入 Call K1（低行权价）+ 卖出 Call K2（高行权价）
  net_debit = p1 - p2（开仓净付出）
  到期结算：
    S ≥ K2      → 两腿均行权，最大盈利 = K2-K1-net_debit，lifecycle 结束  ✓
    K1 ≤ S < K2 → 长腿行权（以 K1 买入正股），短腿虚值作废              → 转入 Phase 2
    S < K1      → 两腿均虚值，最大亏损 = net_debit，lifecycle 结束 ✗
  中途平仓（止损 / 价格检查 / 中期检查 / 早止盈 / 后期止盈）：lifecycle 结束

Phase 2 — Covered Call + 自动滚仓（逻辑同 UTurnBSync_CreditPutSpreadEx）：
  进入条件：Phase 1 K1 ≤ S < K2 到期，长腿行权接货
  有效正股成本：S0 = K1（long call 行权价）
  net_debit 作为成本纳入最终盈亏：
    final_pnl = (stock_exit - S0) - net_debit + prev_premiums + (call_prem - call_close)
  逐步卖出 OTM Call → call 虚值到期时保留正股并开新 call（roll 参数独立）
  直到：行权 / cc_early_profit / max_roll_count

合成订单设计（lifecycle 整体配对，无 FIFO 错配）：
  开仓         → LONG  @ net_debit
  Phase 1 平仓 → SHORT @ payoff（到期）或 SHORT @ curr_spread（中途）
  Phase 2 平仓 → SHORT @ (stock_exit - K1) + prev_premiums + (call_prem - call_close)
  验证：SHORT - LONG = final_pnl ✓

final_pnl（Phase 2 最终平仓）：
  = (stock_exit - S0) - net_debit + prev_premiums + (call_prem - call_close)
"""
from datetime import timedelta, datetime, time as dtime

from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies._cc_dip_mixin import CoveredCallDipMixin
from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps, _load_bars
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing


class UTurnBSync_DebitCallSpreadEx(CoveredCallDipMixin, Strategy):
    """Debit Call Spread → 到期 K1≤S<K2 时长腿行权接货，转入 Covered Call 滚仓。"""

    options_mode:    bool  = True
    initial_capital: float = 100_000.0

    sync_signals:       list  = []
    start_time:         str   = "2024-01-01 00:00:00"
    end_time:           str   = "2026-06-01 00:00:00"
    uturn_btm_level:    int   = 1
    multi_uturn_window: int   = 50
    min_dif_abs:        float = 0.0

    # ── Phase 1: 价差开仓参数 ──────────────────────────────────────────────────
    entry_long:     float = 0.7    # 买入腿 call delta（K1，较低行权价）
    entry_short:    float = 0.3    # 卖出腿 call delta（K2，较高行权价）
    option_days:    int   = 14     # 价差 DTE
    expiry_weekday: int   = -1

    # ── Phase 1: 价差平仓参数 ──────────────────────────────────────────────────
    hard_stop_points:     float = 0.0   # 标的下跌此点数触发硬止损（0=不启用）
    price_check_hours:    float = 0.0   # 开仓后多少小时做一次性价格检查（0=不启用）
    min_price_gain_ratio: float = 0.0   # 价格检查：标的涨幅低于此比例则平仓（0=不限制）
    mid_min_profit_ratio: float = 0.0   # 中期检查：利润低于此比例×max_profit则平仓（支持负值）
    mid_time_ratio:       float = 0.0   # 中期检查：单次检查时间点（0=不启用）
    early_profit_ratio:   float = 0.0   # 提前止盈：利润达到 max_profit 此比例时触发（0=不启用）
    early_time_ratio:     float = 0.0   # 提前止盈：已用时间/总时间 < 此比例时触发（0=不启用）
    late_profit_ratio:    float = 0.0   # 后期止盈：利润达到 max_profit 此比例时触发（0=不启用）
    late_time_ratio:      float = 0.0   # 后期止盈：已用时间/总时间 ≥ 此比例时触发（0=不启用）
    stop_loss_ratio:      float = 0.0   # 权利金止损：损失超过 net_debit×此比例时平仓（0=不启用）

    # ── Phase 2: 滚仓 call 参数（独立） ───────────────────────────────────────
    roll_entry_short:    float = 0.3    # 滚仓 call delta
    roll_option_days:    int   = 14     # 滚仓 call DTE
    roll_expiry_weekday: int   = -1
    max_roll_count:      int   = 0      # 0=不限制

    # ── Phase 2: CC 平仓参数 ───────────────────────────────────────────────────
    cc_roll_profit_ratio:  float = 0.0  # short call 权利金实现率达此比例时买回展期（0=不启用）
    cc_early_profit_ratio: float = 0.0  # total_pnl ≥ ratio×max_profit（早期）（0=不启用）
    cc_early_time_ratio:   float = 0.0

    # ── Dip 参数（UTurnBtm→平腿/买回CC；UTurnTop→再开仓/重建CC）─────────────
    dip_close_level:    int  = 0     # 0=不启用
    dip_close_sync:     bool = True  # True=全部sync_signals；False=单品种
    dip_close_code_id:  str  = ""    # dip_close_sync=False 时有效
    dip_close_window:   int  = 50
    dip_reopen_level:   int  = 0     # 0=立即重建；>0=等待 UTurnTop 信号
    dip_reopen_sync:    bool = True
    dip_reopen_code_id: str  = ""
    dip_reopen_window:  int  = 50
    dip_roll_option_days: int = 0   # 滚动态 dip 重建 call 的到期天数（0=使用 roll_option_days）
    dip_apply_to_spread: bool = False  # False=仅CC阶段；True=价差阶段也适用

    # ── 共用参数 ───────────────────────────────────────────────────────────────
    min_vix:        float = 0.0    # 开仓时 VIX 须 ≥ 此值（0=不限制）
    vix_code_id:    str   = "NQ"   # VIX 来源品种（空串=用交易标的自身 index_close）
    risk_free_rate: float = 0.04
    strike_round:   float = 100.0
    cooldown_hours: float = 1.0
    max_positions:  int   = 3

    # ── 生命周期 ──────────────────────────────────────────────────────────────

    def on_init(self) -> None:
        self._positions: list[dict] = []
        self._closed_positions: list[dict] = []
        self._roll_log: list[dict] = []
        self._position_seq = 0
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._sync_timestamps: set = set()
        self._dip_close_timestamps: set = set()
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
                self.dip_close_level, self.dip_close_window,
            )
        if self.dip_reopen_level > 0:
            self._dip_reopen_timestamps = self._build_top_timestamps(
                self.dip_reopen_sync, self.dip_reopen_code_id,
                self.dip_reopen_level, self.dip_reopen_window,
            )
        self._vix_map: dict = {}
        if self.min_vix > 0 and self.vix_code_id:
            vix_bars = _load_bars(self.vix_code_id, self.interval.value,
                                  self.start_time, self.end_time)
            self._vix_map = {
                b.datetime: b.index_close
                for b in vix_bars if b.index_valid and b.index_close > 0
            }
            self.logger.info(f"VIX 查找表构建完毕: {self.vix_code_id} {len(self._vix_map)} 条记录")

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for p in list(self._positions):
            self._force_close(p, self._last_bar)
        self._positions.clear()
        # 清仓后重置 MTM 为 0，避免 equity 曲线在最后一 bar 出现双重计数
        # （on_stop 在 logic() 之后执行，_mtm_pnl_series[last_bar] 此前已记录浮动盈亏，
        #  而 force_close 又在同一 bar 结算实现盈亏，二者相加会造成重复）
        self._record_mtm_pnl(self._last_bar)

    # ── 通用工具 ──────────────────────────────────────────────────────────────

    def _next_expiry(self, bar_dt: datetime, min_days: int, weekday: int) -> datetime:
        target = bar_dt + timedelta(days=min_days)
        if weekday == -1 or min_days == 1:
            extra = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        else:
            days_ahead = (weekday - target.weekday()) % 7
            expiry_date = (target + timedelta(days=days_ahead)).date()
        return datetime.combine(expiry_date, dtime(16, 0))

    def _rem_t(self, bar_dt: datetime, expiry: datetime) -> float:
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    def _force_close(self, p: dict, bar: BarData) -> None:
        if p['phase'] == 'spread':
            curr_spread = self._spread_curr_price(bar, p)
            self._finalize_spread(p, bar, 'force_close', curr_spread - p['net_debit'])
        else:
            self._cc_close_all(p, bar, 'force_close')

    # ── Dip 动作：价差阶段（覆盖 CoveredCallDipMixin 的 no-op 钩子）────────────

    def _dip_close_spread_leg(self, p: dict, bar: BarData) -> None:
        """买回 short call K2，net_debit 增加（锁住买回成本），进入 spread_dip_waiting。"""
        S = bar.close_price
        T = self._rem_t(bar.datetime, p['spread_expiry'])
        curr_p2 = option_pricing.call_price(
            S, p['k2'], T, self.risk_free_rate, p['iv_short'], self.codeId)
        p['net_debit']          += curr_p2   # 买回 short call 需付出，增加净成本
        p['spread_dip_waiting']  = True
        self.logger.info(
            f"[{bar.datetime}] DipClose spread: 买回 short K2={p['k2']:.2f} "
            f"curr_p2={curr_p2:.4f} → net_debit={p['net_debit']:.4f}"
        )

    def _dip_reopen_spread_leg(self, p: dict, bar: BarData) -> None:
        """在原始 K2 重新卖出 short call，net_debit 减少。"""
        S = bar.close_price
        T = self._rem_t(bar.datetime, p['spread_expiry'])
        p2_new = option_pricing.call_price(
            S, p['k2'], T, self.risk_free_rate, p['iv_short'], self.codeId)
        p['net_debit']          -= p2_new    # 卖出 short call 收到权利金
        p['short_prem']          = p2_new
        p['spread_dip_waiting']  = False
        self.logger.info(
            f"[{bar.datetime}] DipReopen spread: 再卖 short K2={p['k2']:.2f}(原始) "
            f"p2_new={p2_new:.4f} → net_debit={p['net_debit']:.4f}"
        )

    # ── Phase 1: 价差工具 ─────────────────────────────────────────────────────

    def _spread_curr_price(self, bar: BarData, p: dict, S: float = None) -> float:
        """当前价差市值（curr_p1 - curr_p2）。spread_dip_waiting 时仅返回 long call K1 价值。"""
        if S is None:
            S = bar.close_price
        T     = self._rem_t(bar.datetime, p['spread_expiry'])
        curr1 = option_pricing.call_price(S, p['k1'], T, self.risk_free_rate, p['iv_long'],  self.codeId)
        if p.get('spread_dip_waiting', False):
            return curr1   # short leg 已买回，只剩 long call K1
        curr2 = option_pricing.call_price(S, p['k2'], T, self.risk_free_rate, p['iv_short'], self.codeId)
        return curr1 - curr2

    def _finalize_spread(self, p: dict, bar: BarData,
                         close_reason: str, final_pnl: float) -> None:
        """结束 Phase 1 lifecycle，记录并发出合成 SHORT 订单。
        exit_price = net_debit + final_pnl，保证 SHORT - LONG(net_debit) = final_pnl。
        """
        exit_price = p['net_debit'] + final_pnl
        self._send_order(Direction.SHORT, exit_price, 1, "MKT")
        self._closed_positions.append({
            'position_id':      p['position_id'], 'phase': 'spread',
            'entry_dt':         p['entry_dt'],
            'settlement_dt':    bar.datetime,
            'entry_underlying': p['entry_underlying'],
            'k1':               p['k1'],
            'k2':               p['k2'],
            'net_debit':        p['net_debit'],
            'close_reason':     close_reason,
            'final_pnl':        final_pnl,
        })
        self.logger.info(
            f"[{bar.datetime}] Spread平仓 {close_reason} "
            f"K1={p['k1']} K2={p['k2']} pnl={final_pnl:.4f}"
        )

    # ── Phase 1: 到期结算 & 转仓 ──────────────────────────────────────────────

    def _settle_spread_expiry(self, p: dict, bar: BarData) -> bool:
        """到期结算。True=转入 Phase 2（position 保留），False=lifecycle 结束。
        spread_dip_waiting=True 时 short K2 腿已买回，S>=K2 不再触发最大盈利上限。
        """
        S, K1, K2 = bar.close_price, p['k1'], p['k2']
        if not p.get('spread_dip_waiting', False) and S >= K2:
            # 两腿均行权：最大盈利（仅在 short leg 存在时才有上限）
            self._finalize_spread(p, bar, 'expiry_max_profit', (K2 - K1) - p['net_debit'])
            return False
        if S < K1:
            # 两腿均虚值：最大亏损
            self._finalize_spread(p, bar, 'expiry_max_loss', -p['net_debit'])
            return False
        # K1 ≤ S（< K2 或 spread_dip_waiting）：长腿行权（以 K1 买入正股），转入 Phase 2
        self._transition_to_cc(p, bar)
        return True

    def _transition_to_cc(self, p: dict, bar: BarData) -> None:
        """将 spread position 就地改写为 covered_call phase。
        进入条件：K1 ≤ S < K2，长腿行权接货（S0=K1），短腿虚值作废。
        net_debit 留存在 position dict 中，供 Phase 2 pnl 计算扣除。
        """
        S = bar.close_price
        p['phase']         = 'covered_call'
        p['S0']            = p['k1']
        p['prev_premiums'] = 0.0
        p['roll_count']    = 0
        p.update(self._make_call_fields(bar, S, self.roll_entry_short,
                                        self.roll_option_days, self.roll_expiry_weekday))
        self._append_roll_history(p, bar, 'cc_open',
                                  new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                  new_expiry=p['call_expiry'])
        self.logger.info(
            f"[{bar.datetime}] 转仓→CC: S={S:.2f} K1(S0)={p['S0']:.2f} K2={p['k2']:.2f} "
            f"net_debit={p['net_debit']:.4f} "
            f"call_k={p['call_k']} exp={p['call_expiry'].date()}"
        )

    def _check_spread_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in list(self._positions):
            if p['phase'] == 'spread' and bar.datetime >= p['spread_expiry']:
                if self._settle_spread_expiry(p, bar):
                    remaining.append(p)   # 已转仓，保留
            else:
                remaining.append(p)
        self._positions = remaining

    # ── Phase 1: 中途平仓检查 ─────────────────────────────────────────────────

    def _check_spread_hard_stop(self, bar: BarData) -> None:
        if self.hard_stop_points <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread':
                remaining.append(p)
                continue
            stop_price = p['entry_underlying'] - self.hard_stop_points
            if bar.low_price < stop_price:
                curr_spread = self._spread_curr_price(bar, p, stop_price)
                self._finalize_spread(p, bar, 'hard_stop', curr_spread - p['net_debit'])
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_spread_stop_loss(self, bar: BarData) -> None:
        if self.stop_loss_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread' or p.get('spread_dip_waiting', False):
                remaining.append(p)
                continue
            curr_spread = self._spread_curr_price(bar, p)
            loss = p['net_debit'] - curr_spread
            if loss >= self.stop_loss_ratio * p['net_debit']:
                self._finalize_spread(p, bar, 'stop_loss', curr_spread - p['net_debit'])
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_spread_price_progress(self, bar: BarData) -> None:
        if self.price_check_hours <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread' or p['price_checked']:
                remaining.append(p)
                continue
            elapsed_h = (bar.datetime - p['entry_dt']).total_seconds() / 3600
            if elapsed_h < self.price_check_hours:
                remaining.append(p)
                continue
            p['price_checked'] = True
            gain = (bar.close_price - p['entry_underlying']) / p['entry_underlying']
            if gain < self.min_price_gain_ratio:
                curr_spread = self._spread_curr_price(bar, p)
                self._finalize_spread(p, bar, 'price_progress_stop', curr_spread - p['net_debit'])
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_spread_mid_progress(self, bar: BarData) -> None:
        if self.mid_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread' or p['mid_checked'] or p.get('spread_dip_waiting', False):
                remaining.append(p)
                continue
            total_s   = (p['spread_expiry'] - p['entry_dt']).total_seconds()
            elapsed_s = (bar.datetime - p['entry_dt']).total_seconds()
            if total_s <= 0 or elapsed_s / total_s < self.mid_time_ratio:
                remaining.append(p)
                continue
            p['mid_checked'] = True
            curr_spread = self._spread_curr_price(bar, p)
            curr_profit = curr_spread - p['net_debit']
            if p['max_profit'] > 0 and curr_profit < self.mid_min_profit_ratio * p['max_profit']:
                self._finalize_spread(p, bar, 'mid_progress_stop', curr_profit)
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_spread_early_profit(self, bar: BarData) -> None:
        if self.early_profit_ratio <= 0 or self.early_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread' or p.get('spread_dip_waiting', False):
                remaining.append(p)
                continue
            total_s   = (p['spread_expiry'] - p['entry_dt']).total_seconds()
            elapsed_s = (bar.datetime - p['entry_dt']).total_seconds()
            if total_s <= 0 or elapsed_s / total_s >= self.early_time_ratio:
                remaining.append(p)
                continue
            curr_spread = self._spread_curr_price(bar, p)
            curr_profit = curr_spread - p['net_debit']
            if p['max_profit'] > 0 and curr_profit >= self.early_profit_ratio * p['max_profit']:
                self._finalize_spread(p, bar, 'early_profit', curr_profit)
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_spread_late_profit(self, bar: BarData) -> None:
        if self.late_profit_ratio <= 0 or self.late_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread' or p.get('spread_dip_waiting', False):
                remaining.append(p)
                continue
            total_s   = (p['spread_expiry'] - p['entry_dt']).total_seconds()
            elapsed_s = (bar.datetime - p['entry_dt']).total_seconds()
            if total_s <= 0 or elapsed_s / total_s < self.late_time_ratio:
                remaining.append(p)
                continue
            curr_spread = self._spread_curr_price(bar, p)
            curr_profit = curr_spread - p['net_debit']
            if p['max_profit'] > 0 and curr_profit >= self.late_profit_ratio * p['max_profit']:
                self._finalize_spread(p, bar, 'late_profit', curr_profit)
            else:
                remaining.append(p)
        self._positions = remaining

    # ── Phase 2: CC 工具 ──────────────────────────────────────────────────────

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

    def _cc_max_profit(self, p: dict) -> float:
        """(call_k - S0) - net_debit + prev_premiums + call_prem。"""
        return (p['call_k'] - p['S0']) - p['net_debit'] + p['prev_premiums'] + p['call_prem']

    def _cc_total_pnl(self, bar: BarData, p: dict, S: float = None):
        """返回 (total_pnl, curr_call_price)。"""
        if S is None:
            S = bar.close_price
        T         = self._rem_t(bar.datetime, p['call_expiry'])
        curr_call = option_pricing.call_price(
            S, p['call_k'], T, self.risk_free_rate, p['call_iv'], self.codeId)
        pnl = ((S - p['S0']) - p['net_debit'] + p['prev_premiums']
               + (p['call_prem'] - curr_call))
        return pnl, curr_call

    def _cc_elapsed_ratio(self, bar: BarData, p: dict) -> float:
        total   = (p['call_expiry'] - p['call_entry_dt']).total_seconds()
        elapsed = (bar.datetime - p['call_entry_dt']).total_seconds()
        return elapsed / total if total > 0 else 1.0

    def _cc_record(self, p: dict, bar: BarData, close_reason: str,
                   stock_exit: float, call_close: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'position_id':      p['position_id'], 'phase': 'covered_call',
            'close_reason':     close_reason,
            'entry_dt':         p['entry_dt'],
            'settlement_dt':    bar.datetime,
            'entry_underlying': p['entry_underlying'],
            'k1':               p['k1'],
            'k2':               p['k2'],
            'net_debit':        p['net_debit'],
            'S0':               p['S0'],
            'roll_count':       p['roll_count'],
            'prev_premiums':    p['prev_premiums'],
            'call_k':           p['call_k'],
            'call_prem':        p['call_prem'],
            'call_close':       call_close,
            'stock_exit':       stock_exit,
            'final_pnl':        final_pnl,
        })

    def _cc_finalize(self, p: dict, bar: BarData, close_reason: str,
                     stock_exit: float, call_close: float) -> None:
        """平仓 CC phase，发出合成 SHORT 订单（与 entry LONG @ net_debit 配对）。
        exit_price = (stock_exit - K1) + prev_premiums + (call_prem - call_close)
        SHORT - LONG = exit_price - net_debit = final_pnl ✓
        """
        final_pnl  = ((stock_exit - p['S0']) - p['net_debit'] + p['prev_premiums']
                      + (p['call_prem'] - call_close))
        exit_price = ((stock_exit - p['k1']) + p['prev_premiums']
                      + (p['call_prem'] - call_close))
        self._send_order(Direction.SHORT, exit_price, 1, "MKT")
        self._cc_record(p, bar, close_reason, stock_exit, call_close, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] CC平仓 {close_reason} rolls={p['roll_count']} "
            f"S0={p['S0']:.2f} exit={stock_exit:.2f} "
            f"net_debit={p['net_debit']:.4f} pnl={final_pnl:.4f}"
        )

    def _cc_close_all(self, p: dict, bar: BarData, close_reason: str,
                      stock_price: float = None) -> None:
        S = stock_price if stock_price is not None else bar.close_price
        if p.get('dip_waiting', False):
            # call 已买回，call_close=0 表示无 short call 敞口
            self._cc_finalize(p, bar, close_reason, S, 0.0)
        else:
            _, curr_call = self._cc_total_pnl(bar, p, S)
            self._cc_finalize(p, bar, close_reason, S, curr_call)

    # ── Phase 2: CC 到期结算 & 滚仓 ───────────────────────────────────────────

    def _cc_settle_expiry(self, p: dict, bar: BarData) -> bool:
        """True=继续持仓（滚仓），False=lifecycle 结束。"""
        S, K = bar.close_price, p['call_k']
        if S >= K:
            self._cc_finalize(p, bar, 'expiry_assignment', K, 0.0)
            return False
        # call 虚值 → 锁定权利金，滚仓
        p['prev_premiums'] += p['call_prem']
        p['roll_count']    += 1
        if self.max_roll_count > 0 and p['roll_count'] > self.max_roll_count:
            self._cc_finalize(p, bar, 'max_roll_exceeded', S, 0.0)
            return False
        old_k, old_prem = p['call_k'], p['call_prem']
        p.update(self._make_call_fields(bar, S, self.roll_entry_short,
                                        self.roll_option_days, self.roll_expiry_weekday))
        self._append_roll_history(p, bar, 'expiry_otm_roll',
                                  old_call_k=old_k, old_call_prem=old_prem, buyback_price=0.0,
                                  new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                  new_expiry=p['call_expiry'])
        self.logger.info(
            f"[{bar.datetime}] CC滚仓#{p['roll_count']} "
            f"call_k={p['call_k']} prem={p['call_prem']:.4f} "
            f"prev={p['prev_premiums']:.4f} exp={p['call_expiry'].date()}"
        )
        return True

    def _check_cc_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in list(self._positions):
            if (p['phase'] == 'covered_call'
                    and not p.get('dip_waiting', False)
                    and bar.datetime >= p['call_expiry']):
                if self._cc_settle_expiry(p, bar):
                    remaining.append(p)
            else:
                remaining.append(p)
        self._positions = remaining

    # ── Phase 2: CC 权利金展期 ────────────────────────────────────────────────

    def _check_cc_roll_profit(self, bar: BarData) -> None:
        """short call 权利金实现率 ≥ cc_roll_profit_ratio 时买回并立即展期。"""
        if self.cc_roll_profit_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'covered_call' or p.get('dip_waiting', False) or p['call_prem'] <= 0:
                remaining.append(p)
                continue
            S         = bar.close_price
            T         = self._rem_t(bar.datetime, p['call_expiry'])
            curr_call = option_pricing.call_price(
                S, p['call_k'], T, self.risk_free_rate, p['call_iv'], self.codeId)
            realized  = (p['call_prem'] - curr_call) / p['call_prem']
            if realized < self.cc_roll_profit_ratio:
                remaining.append(p)
                continue
            new_roll = p['roll_count'] + 1
            if self.max_roll_count > 0 and new_roll > self.max_roll_count:
                self._cc_finalize(p, bar, 'max_roll_exceeded', S, curr_call)
            else:
                old_k, old_prem = p['call_k'], p['call_prem']
                p['prev_premiums'] += p['call_prem'] - curr_call
                p['roll_count']     = new_roll
                p.update(self._make_call_fields(bar, S, self.roll_entry_short,
                                                self.roll_option_days, self.roll_expiry_weekday))
                self._append_roll_history(p, bar, 'profit_roll',
                                          old_call_k=old_k, old_call_prem=old_prem,
                                          buyback_price=curr_call,
                                          new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                          new_expiry=p['call_expiry'])
                self.logger.info(
                    f"[{bar.datetime}] CC权利金展期#{new_roll} "
                    f"realized={realized:.1%} → call_k={p['call_k']} "
                    f"prem={p['call_prem']:.4f} exp={p['call_expiry'].date()}"
                )
                remaining.append(p)
        self._positions = remaining

    # ── Phase 2: CC 早止盈 ────────────────────────────────────────────────────

    def _check_cc_early_profit(self, bar: BarData) -> None:
        if self.cc_early_profit_ratio <= 0 or self.cc_early_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'covered_call' or p.get('dip_waiting', False):
                remaining.append(p)
                continue
            max_p  = self._cc_max_profit(p)
            pnl, _ = self._cc_total_pnl(bar, p)
            if (max_p > 0
                    and self._cc_elapsed_ratio(bar, p) < self.cc_early_time_ratio
                    and pnl >= self.cc_early_profit_ratio * max_p):
                self._cc_close_all(p, bar, 'cc_early_profit')
            else:
                remaining.append(p)
        self._positions = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        if not self._positions:
            return 0.0
        S, total = bar.close_price, 0.0
        for p in self._positions:
            if p['phase'] == 'spread':
                # _spread_curr_price 内部已处理 spread_dip_waiting
                total += self._spread_curr_price(bar, p, S) - p['net_debit']
            elif p.get('dip_waiting', False):
                # call 已买回，只剩正股多头（S-S0）加上已实现权利金
                total += (S - p['S0']) - p['net_debit'] + p['prev_premiums']
            else:
                pnl, _ = self._cc_total_pnl(bar, p)
                total  += pnl
        return total

    # ── 开仓 ──────────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if bar.datetime not in self._sync_timestamps:
            return False
        if self.min_dif_abs > 0 and abs(bar.dif) < self.min_dif_abs:
            return False
        if self.min_vix > 0:
            vix = (self._vix_map.get(bar.datetime, 0.0) if self.vix_code_id
                   else (bar.index_close if bar.index_valid else 0.0))
            if vix < self.min_vix:
                return False
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        return not (self.max_positions > 0 and len(self._positions) >= self.max_positions)

    def _execute_spread_entry(self, bar: BarData) -> None:
        expiry   = self._next_expiry(bar.datetime, self.option_days, self.expiry_weekday)
        T        = self._rem_t(bar.datetime, expiry)
        iv_long  = get_iv(self.codeId, 'call', self.entry_long)
        iv_short = get_iv(self.codeId, 'call', self.entry_short)
        k1 = option_pricing.call_strike_by_delta(
            bar.close_price, T, self.entry_long,  iv_long,  self.risk_free_rate, self.codeId, self.strike_round)
        k2 = option_pricing.call_strike_by_delta(
            bar.close_price, T, self.entry_short, iv_short, self.risk_free_rate, self.codeId, self.strike_round)
        p1         = option_pricing.call_price(bar.close_price, k1, T, self.risk_free_rate, iv_long,  self.codeId)
        p2         = option_pricing.call_price(bar.close_price, k2, T, self.risk_free_rate, iv_short, self.codeId)
        net_debit  = p1 - p2
        max_profit = (k2 - k1) - net_debit
        self._last_entry_dt = bar.datetime
        self._position_seq += 1
        self._positions.append({
            'position_id':      self._position_seq, 'phase': 'spread',
            'entry_dt':         bar.datetime,
            'entry_underlying': bar.close_price,
            'k1':               k1,
            'k2':               k2,
            'long_prem':        p1,
            'short_prem':       p2,
            'net_debit':        net_debit,
            'max_profit':       max_profit,
            'spread_expiry':    expiry,
            'iv_long':          iv_long,
            'iv_short':         iv_short,
            'price_checked':    False,
            'mid_checked':      False,
        })
        self._send_order(Direction.LONG, net_debit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓 BullCallSpread K1={k1} K2={k2} "
            f"net_debit={net_debit:.4f} max_profit={max_profit:.4f} exp={expiry.date()}"
        )

    def _check_entry(self, bar: BarData) -> None:
        if self._entry_signal(bar):
            self._execute_spread_entry(bar)

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_spread_expiry(bar)        # Phase 1 到期（可能转仓）
        self._check_cc_expiry(bar)            # Phase 2 call 到期
        self._check_cc_roll_profit(bar)       # Phase 2 权利金展期
        self._check_dip_close(bar)            # UTurnBtm：买回 short call / CC call
        self._check_dip_reopen(bar)           # UTurnTop：重建 short call / CC call
        self._check_spread_hard_stop(bar)
        self._check_spread_stop_loss(bar)
        self._check_spread_price_progress(bar)
        self._check_spread_mid_progress(bar)
        self._check_spread_early_profit(bar)
        self._check_spread_late_profit(bar)
        self._check_cc_early_profit(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
