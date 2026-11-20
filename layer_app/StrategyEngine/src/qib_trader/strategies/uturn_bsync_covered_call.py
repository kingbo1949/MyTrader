"""
UTurnBSync_CoveredCall 策略：
多品种 UTurnBottom 信号同步 → 开仓 Covered Call（买正股 + 卖 OTM Call）→ 持有至到期。
是 UTurnBSync_DebitCallSpread 的变体，用实际持有正股替代 Long Call 腿。

开仓结构：
  buy 1 unit underlying at S_entry
  sell 1 OTM call at strike K（由 entry_short delta 决定，K > S）
  net_cost = S_entry - call_prem（净买入成本）

持仓盈亏（任意时刻）：
  stock_pnl = curr_price - S_entry
  call_pnl  = call_prem - curr_call_price
  total_pnl = stock_pnl + call_pnl

到期结算：
  S < K（虚值，call 废弃）: final_pnl = (S - S_entry) + call_prem
  S ≥ K（实值，正股被行权）: final_pnl = (K - S_entry) + call_prem
  统一写法: final_pnl = min(S, K) - S_entry + call_prem

合成订单用于 FIFO 配对：
  Entry:    LONG  at net_cost = S_entry - call_prem
  Exit:     SHORT at min(S, K)           （到期）
  Exit:     SHORT at curr_S - curr_call  （提前平仓）
"""
from datetime import timedelta, datetime, time as dtime

from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing


class UTurnBSync_CoveredCall(Strategy):
    """
    多品种 UTurnBottom 同步信号 → Covered Call（正股 + Short OTM Call）。
    sync_signals 中所有品种同一 bar 同时触发 UTurnBottom 时开仓。
    short call 到期或提前平仓时，正股一并平仓。
    """

    options_mode:    bool  = True
    initial_capital: float = 100_000.0

    sync_signals:    list  = []
    start_time:      str   = "2024-01-01 00:00:00"
    end_time:        str   = "2026-06-01 00:00:00"

    uturn_btm_level:    int   = 1      # 1=SimpleUBtm 2=DbSimpleBtm 3=MixDbSimpleBtm
    multi_uturn_window: int   = 50
    min_dif_abs:        float = 0.0    # 开仓 bar |dif| 最小值（0=不限制）

    entry_short:    float = 0.3        # Short call delta（OTM，K > S）
    option_days:    int   = 14
    expiry_weekday: int   = -1         # -1=仅跳过周末；0=周一…4=周五
    risk_free_rate: float = 0.04
    strike_round:   float = 100.0
    cooldown_hours: float = 1.0
    max_positions:  int   = 3

    hard_stop_points:     float = 0.0  # 正股下跌超此点数触发硬止损（0=不启用）
    price_check_hours:    float = 0.0  # 开仓后多少小时做一次性价格检查（0=不启用）
    min_price_gain_ratio: float = 0.0  # 价格检查：正股涨幅低于此比例则平仓
    early_profit_ratio:   float = 0.0  # call 衰退 ratio × call_prem 时提前平仓（0=不启用）
    early_time_ratio:     float = 0.0  # 仅在已用时间/总时间低于此比例时触发提前止盈
    late_profit_ratio:    float = 0.0  # 后期止盈：call 衰退 ratio × call_prem 时平仓（0=不启用）
    late_time_ratio:      float = 0.0  # 后期止盈：已用时间/总时间高于此比例时才触发
    stop_loss_ratio:      float = 0.0  # 整体亏损超过 ratio × call_prem 时平仓（0=不启用）

    # ── 生命周期 ──────────────────────────────────────────────────────────────

    def on_init(self) -> None:
        self._positions: list[dict] = []
        self._closed_positions: list[dict] = []
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._sync_timestamps: set = set()

    def on_start(self) -> None:
        self.active = True
        if not self.sync_signals:
            self.logger.warning("sync_signals 为空，策略永远不会开仓")
            return
        self.logger.info(f"构建同步时间戳，品种数: {len(self.sync_signals)}")
        self._sync_timestamps = _build_sync_timestamps(
            self.sync_signals, self.interval.value,
            self.start_time, self.end_time,
            level=self.uturn_btm_level, window=self.multi_uturn_window,
        )
        level_names = {1: 'SimpleUBtm', 2: 'DbSimpleBtm', 3: 'MixDbSimpleBtm'}
        self.logger.info(
            f"UTurnBottom 同步时间戳共 {len(self._sync_timestamps)} 个"
            f"（等级: {level_names.get(self.uturn_btm_level, self.uturn_btm_level)}）"
        )

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for p in list(self._positions):
            self._settle_position(p, self._last_bar, 'force_close')
        self._positions.clear()

    # ── 工具 ──────────────────────────────────────────────────────────────────

    def _call_strike_by_delta(self, S: float, T: float, delta: float, iv: float) -> float:
        return option_pricing.call_strike_by_delta(
            S, T, delta, iv, self.risk_free_rate, self.codeId, self.strike_round)

    def _next_expiry(self, bar_dt: datetime, min_days: int) -> datetime:
        if min_days == 1:
            target = bar_dt + timedelta(days=1)
            extra  = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        elif self.expiry_weekday == -1:
            target = bar_dt + timedelta(days=min_days)
            extra  = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        else:
            target = bar_dt + timedelta(days=min_days)
            days_ahead  = (self.expiry_weekday - target.weekday()) % 7
            expiry_date = (target + timedelta(days=days_ahead)).date()
        return datetime.combine(expiry_date, dtime(16, 0))

    def _rem_t(self, bar_dt: datetime, expiry: datetime) -> float:
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    def _curr_call_price(self, bar: BarData, p: dict) -> float:
        T = self._rem_t(bar.datetime, p['expiry'])
        return option_pricing.call_price(bar.close_price, p['k'], T,
                                         self.risk_free_rate, p['iv'], self.codeId)

    # ── 持仓管理 ──────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in self._positions:
            if bar.datetime >= p['expiry']:
                self._settle_position(p, bar)
            else:
                remaining.append(p)
        self._positions = remaining

    def _settle_position(self, p: dict, bar: BarData, close_reason: str = 'expiry') -> None:
        """到期结算：
        S < K → 正股按市价平，call 废弃（keep call_prem）。
        S >= K → 正股在 K 被行权卖出，call_pnl = call_prem - (S - K)。
        统一：effective_stock = min(S, K)；SHORT 合成订单 @ effective_stock。
        """
        S             = bar.close_price
        effective_S   = min(S, p['k'])            # 正股有效卖出价
        call_settle   = max(S - p['k'], 0.0)     # short call 被行权的内在价值损失
        stock_pnl     = effective_S - p['entry_underlying']
        call_pnl      = p['call_prem'] - call_settle
        final_pnl     = stock_pnl + call_pnl
        self._send_order(Direction.SHORT, effective_S, 1, "MKT")
        self._record_position(p, bar, close_reason, effective_S, call_settle, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] CoveredCall到期 K={p['k']} S={S:.2f} "
            f"effective_S={effective_S:.2f} call_settle={call_settle:.2f} "
            f"stock_pnl={stock_pnl:.2f} call_pnl={call_pnl:.2f} final={final_pnl:.2f}"
        )

    def _record_position(self, p: dict, bar: BarData, close_reason: str,
                         stock_settle: float, call_settle: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':         p['entry_dt'],
            'entry_underlying': p['entry_underlying'],
            'settlement_dt':    bar.datetime,
            'close_reason':     close_reason,
            'k':                p['k'],
            'call_prem':        p['call_prem'],
            'stock_settle':     stock_settle,
            'call_settle':      call_settle,
            'stock_pnl':        stock_settle - p['entry_underlying'],
            'call_pnl':         p['call_prem'] - call_settle,
            'final_pnl':        final_pnl,
        })

    def _close_position(self, p: dict, bar: BarData, close_reason: str) -> None:
        """提前平仓：正股按当前价平，call 按当前市价回购。"""
        S          = bar.close_price
        curr_call  = self._curr_call_price(bar, p)
        stock_pnl  = S - p['entry_underlying']
        call_pnl   = p['call_prem'] - curr_call
        final_pnl  = stock_pnl + call_pnl
        self._send_order(Direction.SHORT, S - curr_call, 1, "MKT")
        self._record_position(p, bar, close_reason, S, curr_call, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] {close_reason} K={p['k']} S={S:.2f} "
            f"curr_call={curr_call:.4f} stock_pnl={stock_pnl:.2f} "
            f"call_pnl={call_pnl:.2f} final={final_pnl:.2f}"
        )

    def _check_hard_stop(self, bar: BarData) -> None:
        """正股下跌超过 hard_stop_points 点时，强制平仓整个头寸。"""
        if self.hard_stop_points <= 0:
            return
        remaining = []
        for p in self._positions:
            if bar.low_price < p['entry_underlying'] - self.hard_stop_points:
                self._close_position(p, bar, 'hard_stop')
                self._last_entry_dt = bar.datetime
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_stop_loss(self, bar: BarData) -> None:
        """整体亏损超过 stop_loss_ratio × call_prem 时平仓。"""
        if self.stop_loss_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            curr_call = self._curr_call_price(bar, p)
            total_pnl = (bar.close_price - p['entry_underlying']) + (p['call_prem'] - curr_call)
            if total_pnl < -self.stop_loss_ratio * p['call_prem']:
                self._close_position(p, bar, 'stop_loss')
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_price_progress(self, bar: BarData) -> None:
        """开仓后 price_check_hours 小时，正股涨幅不足 min_price_gain_ratio 时平仓。"""
        if self.price_check_hours <= 0:
            return
        remaining = []
        for p in self._positions:
            elapsed_h = (bar.datetime - p['entry_dt']).total_seconds() / 3600
            if elapsed_h < self.price_check_hours or p['price_checked']:
                remaining.append(p)
                continue
            p['price_checked'] = True
            gain_ratio = (bar.close_price - p['entry_underlying']) / p['entry_underlying']
            if gain_ratio < self.min_price_gain_ratio:
                self._close_position(p, bar, 'price_progress_stop')
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_early_profit(self, bar: BarData) -> None:
        """提前止盈：early_time_ratio 时间内，call 衰退 ≥ early_profit_ratio × call_prem 时平仓。"""
        if self.early_profit_ratio <= 0 or self.early_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            total_secs   = (p['expiry'] - p['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - p['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs >= self.early_time_ratio:
                remaining.append(p)
                continue
            curr_call   = self._curr_call_price(bar, p)
            call_profit = p['call_prem'] - curr_call
            if p['call_prem'] > 0 and call_profit >= self.early_profit_ratio * p['call_prem']:
                self._close_position(p, bar, 'early_profit')
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_late_profit(self, bar: BarData) -> None:
        """后期止盈：late_time_ratio 后，call 衰退 ≥ late_profit_ratio × call_prem 时平仓。"""
        if self.late_profit_ratio <= 0 or self.late_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            total_secs   = (p['expiry'] - p['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - p['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs < self.late_time_ratio:
                remaining.append(p)
                continue
            curr_call   = self._curr_call_price(bar, p)
            call_profit = p['call_prem'] - curr_call
            if p['call_prem'] > 0 and call_profit >= self.late_profit_ratio * p['call_prem']:
                self._close_position(p, bar, 'late_profit')
            else:
                remaining.append(p)
        self._positions = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        """Covered Call MTM：Σ ((curr_stock - S_entry) + (call_prem - curr_call))。"""
        if not self._positions:
            return 0.0
        S, total = bar.close_price, 0.0
        for p in self._positions:
            curr_call = self._curr_call_price(bar, p)
            total += (S - p['entry_underlying']) + (p['call_prem'] - curr_call)
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
        if self.max_positions > 0 and len(self._positions) >= self.max_positions:
            return False
        return True

    def _check_entry(self, bar: BarData) -> None:
        """开仓：买正股 + 卖 OTM Call（K 由 entry_short delta 决定）。"""
        if not self._entry_signal(bar):
            return
        S         = bar.close_price
        expiry    = self._next_expiry(bar.datetime, self.option_days)
        T         = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv        = get_iv(self.codeId, 'call', self.entry_short)
        k         = self._call_strike_by_delta(S, T, self.entry_short, iv)
        call_prem = option_pricing.call_price(S, k, T, self.risk_free_rate, iv, self.codeId)
        net_cost  = S - call_prem
        self._last_entry_dt = bar.datetime
        self._positions.append({
            'entry_dt':         bar.datetime,
            'entry_underlying': S,
            'k':                k,
            'call_prem':        call_prem,
            'expiry':           expiry,
            'iv':               iv,
            'price_checked':    False,
        })
        self._send_order(Direction.LONG, net_cost, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓CoveredCall S={S:.2f} K={k} "
            f"call_prem={call_prem:.4f} net_cost={net_cost:.2f} expiry={expiry.date()}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_expiry(bar)
        self._check_hard_stop(bar)
        self._check_stop_loss(bar)
        self._check_price_progress(bar)
        self._check_early_profit(bar)
        self._check_late_profit(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
