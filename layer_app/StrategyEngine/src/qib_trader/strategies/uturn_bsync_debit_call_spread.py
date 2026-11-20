"""
UTurnBSync_DebitCallSpread 策略：
多品种 UTurnBottom 信号同步 → 开仓 Bull Call Spread（Debit Call Spread）→ 持有至到期结算。

uturn_btm_level 控制同步信号等级：
  1 = SimpleUBtm      : is_uturn=True + Bottom/BottomSub
  2 = DbSimpleBtm     : 窗口内 ≥2 个底部 UTurn 点（不要求 MACD>0）
  3 = MixDbSimpleBtm  : DbSimpleBtm + 至少一根 bar 的 MACD>0

开仓结构：
  entry_long  delta 对应的 call 为低行权价 K1（买入腿）
  entry_short delta 对应的 call 为高行权价 K2（卖出腿）
  net_debit  = p1 - p2（开仓时净付出保证金）
  max_profit = K2 - K1 - net_debit（spread 拉满时的最大盈利）
  max_loss   = net_debit
"""
from datetime import timedelta, datetime, time as dtime
from pathlib import Path


from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing

_DATADB = Path(__file__).parents[3] / "datadb"



# ── 策略主体 ──────────────────────────────────────────────────────────────────

class UTurnBSync_DebitCallSpread(Strategy):
    """
    多品种简单UTurnBottom同步信号 → Bull Call Spread。
    sync_signals 中所有品种在同一根 bar 同时触发简单UTurnBottom时开仓。
    """

    options_mode: bool   = True
    initial_capital: float = 100_000.0

    sync_signals:   list  = []
    start_time:     str   = "2024-01-01 00:00:00"
    end_time:       str   = "2026-06-01 00:00:00"

    uturn_btm_level:  int   = 1    # 信号等级：1=SimpleUBtm 2=DbSimpleBtm 3=MixDbSimpleBtm
    multi_uturn_window: int = 50   # Level 2/3 使用的 bar 历史窗口大小
    min_dif_abs:    float = 0.0    # 交易标的开仓 bar 的 |dif| 最小值，低于此值忽略（0=不限制）

    entry_long:     float = 0.4      # 买入腿 call delta（K1，较低行权价，较高 delta）
    entry_short:    float = 0.1      # 卖出腿 call delta（K2，较高行权价，较低 delta）
    option_days:    int   = 5
    expiry_weekday: int   = -1       # 到期日星期几：0=周一…4=周五，-1=仅跳过周末
    risk_free_rate: float = 0.04
    strike_round:   float = 100.0
    cooldown_hours: float = 1.0
    max_positions:  int   = 3
    hard_stop_points:    float = 0.0  # 标的下跌此点数触发硬止损（0=不启用）
    mid_min_profit_ratio: float = 0.0  # 中期检查：利润低于此比例×max_profit则平仓（支持负值表示允许亏损阈值）
    mid_time_ratio:       float = 0.0  # 中期检查：单次检查的时间点（已用时间/总持仓时间，0=不启用）
    price_check_hours:    float = 0.0  # 价格检查：开仓后多少小时做一次性检查（0=不启用）
    min_price_gain_ratio: float = 0.0  # 价格检查：标的涨幅低于此比例则平仓（如 0.002=0.2%，支持负值）
    early_profit_ratio:  float = 0.0  # 提前止盈：利润达到 max_profit 此比例时触发（0=不启用）
    early_time_ratio:    float = 0.0  # 提前止盈：已用时间/总持仓时间低于此比例时才触发（0=不启用）
    late_profit_ratio:   float = 0.0  # 后期止盈：利润达到 max_profit 此比例时触发（0=不启用）
    late_time_ratio:     float = 0.0  # 后期止盈：已用时间/总持仓时间高于此比例时才触发（0=不启用）
    stop_loss_ratio:     float = 0.0  # 权利金止损：当前价差价值亏损超过 net_debit×此比例时平仓（0=不启用）

    def on_init(self) -> None:
        self._spreads: list[dict] = []
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
            f"UTurnBottom同步时间戳共 {len(self._sync_timestamps)} 个"
            f"（等级: {level_names.get(self.uturn_btm_level, self.uturn_btm_level)}）"
        )

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for sp in self._spreads:
            self._settle_spread(sp, self._last_bar, 'force_close')
        self._spreads.clear()

    # ── 工具 ──────────────────────────────────────────────────────────────────

    def _call_strike_by_delta(self, S: float, T: float, call_delta: float, iv: float) -> float:
        return option_pricing.call_strike_by_delta(
            S, T, call_delta, iv, self.risk_free_rate, self.codeId, self.strike_round)

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
        expiry = datetime.combine(expiry_date, dtime(16, 0))
        return expiry

    def _rem_t(self, bar_dt: datetime, expiry: datetime) -> float:
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    # ── 持仓管理 ──────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        remaining = []
        for sp in self._spreads:
            if bar.datetime >= sp['expiry']:
                self._settle_spread(sp, bar)
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _settle_spread(self, sp: dict, bar: BarData, close_reason: str = 'expiry') -> None:
        S            = bar.close_price
        long_settle  = max(S - sp['k1'], 0.0)
        short_settle = max(S - sp['k2'], 0.0)
        payoff       = long_settle - short_settle
        final_pnl    = payoff - sp['net_debit']
        self._send_order(Direction.SHORT, payoff, 1, "MKT")
        self._record_position(sp, bar, close_reason, long_settle, short_settle, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] spread到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} final_pnl={final_pnl:.2f}"
        )

    def _record_position(self, sp: dict, bar: BarData, close_reason: str,
                         long_settle: float, short_settle: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':         sp['entry_dt'],
            'entry_underlying': sp['entry_underlying'],
            'settlement_dt':    bar.datetime,
            'close_reason':     close_reason,
            'k1':               sp['k1'],
            'k2':               sp['k2'],
            'long_prem':        sp['long_prem'],
            'short_prem':       sp['short_prem'],
            'net_debit':        sp['net_debit'],
            'max_profit':       sp['max_profit'],
            'long_settle':      long_settle,
            'short_settle':     short_settle,
            'final_pnl':        final_pnl,
        })

    def _settle_stop_loss(self, sp: dict, bar: BarData,
                          close_reason: str = 'stop_loss',
                          settle_price: float | None = None) -> None:
        T           = self._rem_t(bar.datetime, sp['expiry'])
        S           = settle_price if settle_price is not None else bar.close_price
        curr_p1     = option_pricing.call_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
        curr_p2     = option_pricing.call_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
        curr_spread = curr_p1 - curr_p2
        final_pnl   = curr_spread - sp['net_debit']
        self._send_order(Direction.SHORT, curr_spread, 1, "MKT")
        self._record_position(sp, bar, close_reason, curr_p1, curr_p2, final_pnl)
        self.logger.info(f"[{bar.datetime}] {close_reason} S={S:.2f} "
                         f"K1={sp['k1']} K2={sp['k2']} "
                         f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")

    def _check_stop_loss(self, bar: BarData) -> None:
        """权利金止损：当价差当前价值亏损超过 net_debit × stop_loss_ratio 时平仓。
        例：stop_loss_ratio=0.2，付出 net_debit=100，当价差跌至 80 以下即触发。
        """
        if self.stop_loss_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            T           = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1     = option_pricing.call_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            curr_p2     = option_pricing.call_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_spread = curr_p1 - curr_p2
            loss        = sp['net_debit'] - curr_spread          # 正值表示已亏损
            if loss >= self.stop_loss_ratio * sp['net_debit']:
                self._settle_stop_loss(sp, bar, 'premium_stop_loss')
                self.logger.info(
                    f"[{bar.datetime}] 权利金止损 K1={sp['k1']} K2={sp['k2']} "
                    f"net_debit={sp['net_debit']:.2f} curr_spread={curr_spread:.2f} "
                    f"loss_ratio={loss/sp['net_debit']:.2%}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_hard_stop(self, bar: BarData) -> None:
        if self.hard_stop_points <= 0:
            return
        remaining = []
        for sp in self._spreads:
            stop_price = sp['entry_underlying'] - self.hard_stop_points
            if bar.low_price < stop_price:
                self._settle_stop_loss(sp, bar, close_reason='hard_stop',
                                       settle_price=stop_price)
                self._last_entry_dt = bar.datetime
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_price_progress(self, bar: BarData) -> None:
        if self.price_check_hours <= 0:
            return
        remaining = []
        for sp in self._spreads:
            elapsed_h = (bar.datetime - sp['entry_dt']).total_seconds() / 3600
            if elapsed_h < self.price_check_hours or sp['price_checked']:
                remaining.append(sp)
                continue
            sp['price_checked'] = True
            gain_ratio = (bar.close_price - sp['entry_underlying']) / sp['entry_underlying']
            if gain_ratio < self.min_price_gain_ratio:
                self._settle_stop_loss(sp, bar, 'price_progress_stop')
                self.logger.info(
                    f"[{bar.datetime}] 价格检查止损 K1={sp['k1']} K2={sp['k2']} "
                    f"gain_ratio={gain_ratio:.4f} threshold={self.min_price_gain_ratio:.4f}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_mid_progress(self, bar: BarData) -> None:
        if self.mid_time_ratio <= 0:
            return
        remaining = []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs < self.mid_time_ratio or sp['mid_checked']:
                remaining.append(sp)
                continue
            sp['mid_checked'] = True
            if sp['max_profit'] <= 0:
                remaining.append(sp)
                continue
            T           = self._rem_t(bar.datetime, sp['expiry'])
            S           = bar.close_price
            curr_p1     = option_pricing.call_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
            curr_p2     = option_pricing.call_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_spread = curr_p1 - curr_p2
            curr_profit = curr_spread - sp['net_debit']
            if curr_profit < self.mid_min_profit_ratio * sp['max_profit']:
                self._settle_stop_loss(sp, bar, 'mid_progress_stop')
                self.logger.info(
                    f"[{bar.datetime}] 中期止损 K1={sp['k1']} K2={sp['k2']} "
                    f"curr_profit={curr_profit:.2f} threshold={self.mid_min_profit_ratio * sp['max_profit']:.2f}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_late_profit(self, bar: BarData) -> None:
        if self.late_profit_ratio <= 0 or self.late_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs < self.late_time_ratio:
                remaining.append(sp)
                continue
            if sp['max_profit'] <= 0:
                remaining.append(sp)
                continue
            T           = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1     = option_pricing.call_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
            curr_p2     = option_pricing.call_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_spread = curr_p1 - curr_p2
            curr_profit = curr_spread - sp['net_debit']
            if curr_profit >= self.late_profit_ratio * sp['max_profit']:
                self._send_order(Direction.SHORT, curr_spread, 1, "MKT")
                self._record_position(sp, bar, 'late_profit', curr_p1, curr_p2, curr_profit)
                self.logger.info(f"[{bar.datetime}] 后期止盈 K1={sp['k1']} K2={sp['k2']} "
                                 f"curr_spread={curr_spread:.2f} final_pnl={curr_profit:.2f}")
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_early_profit(self, bar: BarData) -> None:
        if self.early_profit_ratio <= 0 or self.early_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs >= self.early_time_ratio:
                remaining.append(sp)
                continue
            if sp['max_profit'] <= 0:
                remaining.append(sp)
                continue
            T           = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1     = option_pricing.call_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
            curr_p2     = option_pricing.call_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_spread = curr_p1 - curr_p2
            curr_profit = curr_spread - sp['net_debit']
            if curr_profit >= self.early_profit_ratio * sp['max_profit']:
                self._send_order(Direction.SHORT, curr_spread, 1, "MKT")
                self._record_position(sp, bar, 'early_profit', curr_p1, curr_p2, curr_profit)
                self.logger.info(f"[{bar.datetime}] 提前止盈 K1={sp['k1']} K2={sp['k2']} "
                                 f"curr_spread={curr_spread:.2f} final_pnl={curr_profit:.2f}")
            else:
                remaining.append(sp)
        self._spreads = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        """Debit Call Spread MTM：Σ (当前价差市值 - 支付的净权利金)，盈正亏负。"""
        if not self._spreads:
            return 0.0
        S, total = bar.close_price, 0.0
        for sp in self._spreads:
            T       = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1 = option_pricing.call_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            curr_p2 = option_pricing.call_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            total  += (curr_p1 - curr_p2) - sp['net_debit']
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
        if self.max_positions > 0 and len(self._spreads) >= self.max_positions:
            return False
        return True

    def _check_entry(self, bar: BarData) -> None:
        if not self._entry_signal(bar):
            return
        expiry     = self._next_expiry(bar.datetime, self.option_days)
        T          = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv_long    = get_iv(self.codeId, 'call', self.entry_long)
        iv_short   = get_iv(self.codeId, 'call', self.entry_short)
        k1         = self._call_strike_by_delta(bar.close_price, T, self.entry_long,  iv_long)
        k2         = self._call_strike_by_delta(bar.close_price, T, self.entry_short, iv_short)
        p1         = option_pricing.call_price(bar.close_price, k1, T, self.risk_free_rate, iv_long, self.codeId)
        p2         = option_pricing.call_price(bar.close_price, k2, T, self.risk_free_rate, iv_short, self.codeId)
        net_debit  = p1 - p2
        max_profit = (k2 - k1) - net_debit
        self._last_entry_dt = bar.datetime
        self._spreads.append({
            'entry_dt':         bar.datetime,
            'entry_underlying': bar.close_price,
            'k1': k1, 'k2': k2,
            'long_prem':  p1, 'short_prem': p2,
            'net_debit':  net_debit,
            'max_profit': max_profit,
            'expiry':     expiry,
            'iv_long':       iv_long, 'iv_short': iv_short,
            'mid_checked':   False,
            'price_checked': False,
        })
        self._send_order(Direction.LONG, net_debit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 同步信号开仓BullCallSpread K1={k1} K2={k2} "
            f"net_debit={net_debit:.2f} max_profit={max_profit:.2f} expiry={expiry.date()}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_expiry(bar)
        self._check_hard_stop(bar)
        self._check_stop_loss(bar)
        self._check_price_progress(bar)
        self._check_mid_progress(bar)
        self._check_early_profit(bar)
        self._check_late_profit(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
