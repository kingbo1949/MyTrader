"""
UTurnBottomCreditPut 策略：UTurnBottom 信号 → 直接开仓 Bull Put Spread → 持有至到期结算。
uturn_btm_level 控制信号等级：
  1 = SimpleUBtm      : is_uturn=True + Bottom/BottomSub
  2 = DbSimpleBtm     : 窗口内 ≥2 个底部 UTurn 点（不要求 MACD>0）
  3 = MixDbSimpleBtm  : DbSimpleBtm + 至少一根 bar 的 MACD>0
"""
from collections import deque
from datetime import timedelta, datetime, time as dtime

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing


class UTurnBottomCreditPut(Strategy):
    """
    UTurnBottom 期权策略（Bull Put Spread），支持三个信号等级，持有至到期结算。
    uturn_btm_level: 1=SimpleUBtm, 2=DbSimpleBtm, 3=MixDbSimpleBtm
    """

    options_mode: bool = True
    initial_capital: float = 100_000.0

    entry_short: float = 0.3
    entry_long: float = 0.1
    option_days: int = 7
    expiry_weekday: int = 2
    risk_free_rate: float = 0.04
    strike_round: float = 100.0
    uturn_btm_level: int = 1              # 信号等级：1=SimpleUBtm 2=DbSimpleBtm 3=MixDbSimpleBtm
    multi_uturn_window: int = 50          # Level 2/3 使用的 bar 历史窗口大小
    min_dif_abs: float = 0.0             # 开仓 bar |dif| 最小阈值，0=不限制
    cooldown_hours: float = 0.0
    max_positions: int = 0
    require_low_below_prev_day_low: int = 0
    require_lower_k1_than_existing: int = 0
    require_plus_pattern: int = 0         # 1=要求当前 UTurn 点|dif|>上一个（Level 2/3 有效）
    hard_stop_points: float = 0.0        # 标的价格从开仓价下跌此点数触发硬止损（0=不启用）
    stop_loss_ratio: float = 0.0         # 亏损达到最大亏损此比例时平仓（0=不启用）
    early_profit_ratio: float = 0.0
    early_time_ratio: float = 0.0
    late_close_time_ratio: float = 0.0  # 时间强制平仓：已用时间/总持仓时间超过此比例时无条件平仓，0=不启用

    def on_init(self) -> None:
        self._spreads: list[dict] = []
        self._closed_positions: list[dict] = []
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._bar_history: deque = deque(maxlen=self.multi_uturn_window)

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for sp in self._spreads:
            self._settle_spread(sp, self._last_bar, 'force_close')
        self._spreads.clear()

    # ── 工具方法 ──────────────────────────────────────────────────────────────

    def _put_strike_by_delta(self, S: float, T: float, put_delta_abs: float, iv: float) -> float:
        return option_pricing.put_strike_by_delta(
            S, T, put_delta_abs, iv, self.risk_free_rate, self.codeId, self.strike_round)

    def _next_expiry(self, bar_dt, min_days: int):
        if min_days == 1:
            target = bar_dt + timedelta(days=1)
            extra = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        elif self.expiry_weekday == -1:
            target = bar_dt + timedelta(days=min_days)
            extra = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        else:
            target = bar_dt + timedelta(days=min_days)
            days_ahead = (self.expiry_weekday - target.weekday()) % 7
            expiry_date = (target + timedelta(days=days_ahead)).date()
        expiry = datetime.combine(expiry_date, dtime(16, 0))
        return expiry

    def _rem_t(self, bar_dt, expiry) -> float:
        delta = (expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600)
        return max(delta, 0.0)

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
        S = bar.close_price
        short_settle = max(sp['k1'] - S, 0.0)
        long_settle  = max(sp['k2'] - S, 0.0)
        payoff = long_settle - short_settle
        net_credit = sp['net_credit']
        final_pnl = net_credit + payoff
        self._send_order(Direction.LONG, net_credit - final_pnl, 1, "MKT")
        self._record_position(sp, bar, close_reason, short_settle, long_settle, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] spread到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} final_pnl={final_pnl:.2f}"
        )

    def _record_position(self, sp: dict, bar: BarData, close_reason: str,
                         short_settle: float, long_settle: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':         sp['entry_dt'],
            'entry_underlying': sp['entry_underlying'],
            'lock_dt':          sp.get('lock_dt'),
            'lock_reason':      sp.get('lock_reason'),
            'settlement_dt':    bar.datetime,
            'close_reason':     close_reason,
            'k1':               sp['k1'],
            'k2':               sp['k2'],
            'short_prem':       sp['short_prem'],
            'long_prem':        sp['long_prem'],
            'net_credit':       sp['net_credit'],
            'short_settle':     short_settle,
            'long_settle':      long_settle,
            'final_pnl':        final_pnl,
        })

    def _settle_stop_loss(self, sp: dict, bar: BarData,
                          close_reason: str = 'stop_loss',
                          settle_price: float | None = None,
                          iv_override: float | None = None) -> None:
        T = self._rem_t(bar.datetime, sp['expiry'])
        S  = settle_price if settle_price is not None else bar.close_price
        iv = iv_override if iv_override is not None else sp['iv_short']
        curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, iv, self.codeId)
        curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, iv, self.codeId)
        curr_spread = curr_p1 - curr_p2
        final_pnl = sp['net_credit'] - curr_spread
        self._send_order(Direction.LONG, curr_spread, 1, "MKT")
        self._record_position(sp, bar, close_reason, 0.0, 0.0, final_pnl)
        self.logger.info(f"[{bar.datetime}] {close_reason} S={S:.2f} iv={iv:.3f} "
                         f"K1={sp['k1']} K2={sp['k2']} "
                         f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")

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

    def _settle_early_profit(self, sp: dict, bar: BarData) -> None:
        T = self._rem_t(bar.datetime, sp['expiry'])
        S = bar.close_price
        curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
        curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
        curr_spread = curr_p1 - curr_p2
        final_pnl = sp['net_credit'] - curr_spread
        self._send_order(Direction.LONG, curr_spread, 1, "MKT")
        self._record_position(sp, bar, 'early_profit', 0.0, 0.0, final_pnl)
        self.logger.info(f"[{bar.datetime}] 提前止盈 K1={sp['k1']} K2={sp['k2']} "
                         f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")

    def _check_early_profit(self, bar: BarData) -> None:
        if self.early_profit_ratio <= 0 or self.early_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs >= self.early_time_ratio:
                remaining.append(sp)
                continue
            T = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
            curr_profit = sp['net_credit'] - (curr_p1 - curr_p2)
            if sp['net_credit'] > 0 and curr_profit >= self.early_profit_ratio * sp['net_credit']:
                self._settle_early_profit(sp, bar)
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _is_plus_pattern(self, zone) -> bool:
        uturn_bars = [b for b in zone.zone_bars
                      if b.div_type in ('Bottom', 'BottomSub') and b.is_uturn]
        if len(uturn_bars) < 2:
            return False
        return abs(uturn_bars[-1].dif) > abs(uturn_bars[-2].dif)

    def _check_late_close(self, bar: BarData) -> None:
        if self.late_close_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs > 0 and elapsed_secs / total_secs >= self.late_close_time_ratio:
                T = self._rem_t(bar.datetime, sp['expiry'])
                curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
                curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
                curr_spread = curr_p1 - curr_p2
                final_pnl = sp['net_credit'] - curr_spread
                self._send_order(Direction.LONG, curr_spread, 1, "MKT")
                self._record_position(sp, bar, 'late_close', 0.0, 0.0, final_pnl)
                self.logger.info(f"[{bar.datetime}] 时间平仓 K1={sp['k1']} K2={sp['k2']} "
                                 f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_stop_loss(self, bar: BarData) -> None:
        if self.stop_loss_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            T = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
            max_loss = (sp['k1'] - sp['k2']) - sp['net_credit']
            loss = (curr_p1 - curr_p2) - sp['net_credit']
            if max_loss > 0 and loss >= self.stop_loss_ratio * max_loss:
                self._settle_stop_loss(sp, bar)
            else:
                remaining.append(sp)
        self._spreads = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        """Bull Put Spread MTM：Σ (收到净权利金 - 当前回购成本)，盈正亏负。"""
        if not self._spreads:
            return 0.0
        S, total = bar.close_price, 0.0
        for sp in self._spreads:
            T       = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            total  += sp['net_credit'] - (curr_p1 - curr_p2)
        return total

    def _entry_signal(self, bar: BarData) -> bool:
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.max_positions > 0 and len(self._spreads) >= self.max_positions:
            return False
        if self.min_dif_abs > 0 and abs(bar.dif) < self.min_dif_abs:
            return False
        if self.require_low_below_prev_day_low and bar.prev_day_low > 0 and bar.low_price >= bar.prev_day_low:
            return False
        if self.uturn_btm_level == 1:
            if not bar.is_uturn or bar.div_type not in ('Bottom', 'BottomSub'):
                return False
        else:
            zone = detect_multi_uturn(list(self._bar_history))
            if zone is None or zone.zone_type != 'bottom':
                return False
            if self.uturn_btm_level == 3 and not zone.is_mix:
                return False
            if self.require_plus_pattern and not self._is_plus_pattern(zone):
                return False
        return True

    def _check_entry(self, bar: BarData) -> None:
        if not self._entry_signal(bar):
            return
        expiry = self._next_expiry(bar.datetime, self.option_days)
        T = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv_short = get_iv(self.codeId, 'put', self.entry_short)
        iv_long  = get_iv(self.codeId, 'put', self.entry_long)
        k1 = self._put_strike_by_delta(bar.close_price, T, self.entry_short, iv_short)
        k2 = self._put_strike_by_delta(bar.close_price, T, self.entry_long,  iv_long)
        if self.require_lower_k1_than_existing and self._spreads and k1 >= min(sp['k1'] for sp in self._spreads):
            return
        p1 = option_pricing.put_price(bar.close_price, k1, T, self.risk_free_rate, iv_short, self.codeId)
        p2 = option_pricing.put_price(bar.close_price, k2, T, self.risk_free_rate, iv_long, self.codeId)
        net_credit = p1 - p2
        self._last_entry_dt = bar.datetime
        self._spreads.append({
            'entry_dt': bar.datetime, 'lock_dt': bar.datetime, 'lock_reason': 'direct_entry',
            'entry_underlying': bar.close_price,
            'k1': k1, 'k2': k2,
            'short_prem': p1, 'long_prem': p2, 'net_credit': net_credit,
            'expiry': expiry,
            'iv_short': iv_short, 'iv_long': iv_long,
        })
        self._send_order(Direction.SHORT, net_credit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓BullPutSpread K1={k1} K2={k2} "
            f"net_credit={net_credit:.2f} expiry={expiry.date()}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._bar_history.append(bar)
        self._check_expiry(bar)
        self._check_hard_stop(bar)
        self._check_stop_loss(bar)
        self._check_early_profit(bar)
        self._check_late_close(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
