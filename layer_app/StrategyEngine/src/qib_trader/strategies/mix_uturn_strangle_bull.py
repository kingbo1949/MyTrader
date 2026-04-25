"""
MixUTurnStrangleBull 策略：底部 Mix UTurn 信号 → 同时买入 OTM Call 和 OTM Put（Long Strangle）→ 持有至到期结算。
entry_call 指定买入 call 的 delta，entry_put 指定买入 put 的 delta 绝对值。
可选 entry_put_short（> 0）：在更低行权价卖出一张 put，构成 Bull Put Spread，降低 put 腿成本。
看涨偏向：call 腿为主盈利方向，put 腿提供下行对冲。
"""
import math
from collections import deque
from datetime import timedelta, datetime, time as dtime
from scipy.stats import norm

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import get_iv


def bs_call_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Call 期权 Black-Scholes 定价（T 为年化剩余时间）。"""
    if T <= 0:
        return max(S - K, 0.0)
    if K <= 0 or sigma <= 0:
        return max(S - K, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


def bs_put_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Put 期权 Black-Scholes 定价（T 为年化剩余时间）。"""
    if T <= 0:
        return max(K - S, 0.0)
    if K <= 0 or sigma <= 0:
        return max(K - S, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return K * math.exp(-r * T) * norm.cdf(-d2) - S * norm.cdf(-d1)


class MixUTurnStrangleBull(Strategy):
    """
    底部 Mix UTurn 期权策略（Long Strangle），触发信号后立即同时买入 call 和 put，持有至到期结算。
    params: risk_free_rate, option_days, entry_call, entry_put,
            expiry_weekday, cooldown_hours, max_positions,
            min_drop_from_prev_close, require_low_below_prev_day_low
    """

    options_mode: bool = True
    initial_capital: float = 100_000.0

    entry_call: float = 0.3            # 买入 OTM call 的 delta（行权价高于现价）
    entry_put: float = 0.3             # 买入 OTM put 的 delta 绝对值（行权价低于现价）
    entry_put_short: float = 0.0       # 卖出 put 的 delta 绝对值（更低行权价），0=不启用
    option_days: int = 7
    expiry_weekday: int = -1           # -1=每日期权；0~4=指定星期几
    risk_free_rate: float = 0.04
    strike_round: float = 100.0
    multi_uturn_window: int = 300
    cooldown_hours: float = 0.0
    max_positions: int = 0
    min_drop_from_prev_close: float = 0.0
    require_low_below_prev_day_low: int = 0

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
            self._settle_strangle(sp, self._last_bar, 'force_close')
        self._spreads.clear()

    # ── 工具方法 ──────────────────────────────────────────────────────────────

    def _call_strike_by_delta(self, S: float, T: float, call_delta: float, iv: float) -> float:
        """由 call delta 反推行权价，取整到最近的 strike_round 点。"""
        d1 = norm.ppf(call_delta)
        K = S * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
        return round(K / self.strike_round) * self.strike_round

    def _put_strike_by_delta(self, S: float, T: float, put_delta_abs: float, iv: float) -> float:
        """由 put delta 绝对值反推行权价，取整到最近的 strike_round 点。"""
        d1 = norm.ppf(1.0 - put_delta_abs)
        K = S * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
        return round(K / self.strike_round) * self.strike_round

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
        return datetime.combine(expiry_date, dtime(16, 0))

    def _rem_t(self, bar_dt, expiry) -> float:
        delta = (expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600)
        return max(delta, 0.0)

    # ── 持仓管理 ──────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        remaining = []
        for sp in self._spreads:
            if bar.datetime >= sp['expiry']:
                self._settle_strangle(sp, bar)
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _settle_strangle(self, sp: dict, bar: BarData, close_reason: str = 'expiry') -> None:
        """到期结算。
        payoff = max(S-K_call,0) + max(K_put-S,0) - max(K_put_short-S,0)
        final_pnl = payoff - net_debit
        """
        S = bar.close_price
        call_settle      = max(S - sp['k_call'], 0.0)
        put_settle       = max(sp['k_put'] - S, 0.0)
        put_short_settle = max(sp['k_put_short'] - S, 0.0) if sp['k_put_short'] > 0 else 0.0
        total_payoff = call_settle + put_settle - put_short_settle
        net_debit = sp['net_debit']
        final_pnl = total_payoff - net_debit
        self._send_order(Direction.SHORT, total_payoff, 1, "MKT")
        self._record_position(sp, bar, close_reason, call_settle, put_settle, put_short_settle, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] strangle到期 K_call={sp['k_call']} K_put={sp['k_put']} "
            f"K_put_short={sp['k_put_short']} payoff={total_payoff:.2f} final_pnl={final_pnl:.2f}"
        )

    def _record_position(self, sp: dict, bar: BarData, close_reason: str,
                         call_settle: float, put_settle: float,
                         put_short_settle: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':        sp['entry_dt'],
            'settlement_dt':   bar.datetime,
            'close_reason':    close_reason,
            'k_call':          sp['k_call'],
            'k_put':           sp['k_put'],
            'k_put_short':     sp['k_put_short'],
            'call_prem':       sp['call_prem'],
            'put_prem':        sp['put_prem'],
            'put_short_prem':  sp['put_short_prem'],
            'net_debit':       sp['net_debit'],
            'call_settle':     call_settle,
            'put_settle':      put_settle,
            'put_short_settle': put_short_settle,
            'final_pnl':       final_pnl,
        })

    # ── 开仓信号 ──────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.max_positions > 0 and len(self._spreads) >= self.max_positions:
            return False
        if self.require_low_below_prev_day_low and bar.prev_day_low > 0 and bar.low_price >= bar.prev_day_low:
            return False
        zone = detect_multi_uturn(list(self._bar_history))
        return zone is not None and zone.zone_type == 'bottom' and zone.is_mix

    def _check_entry(self, bar: BarData) -> None:
        """同时买入 OTM Call 和 OTM Put；若 entry_put_short > 0，额外卖出更低行权价 put 降低成本。"""
        if not self._entry_signal(bar):
            return
        expiry = self._next_expiry(bar.datetime, self.option_days)
        T = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv_call = get_iv(self.codeId, 'call', self.entry_call)
        iv_put  = get_iv(self.codeId, 'put',  self.entry_put)
        k_call  = self._call_strike_by_delta(bar.close_price, T, self.entry_call, iv_call)
        k_put   = self._put_strike_by_delta (bar.close_price, T, self.entry_put,  iv_put)
        p_call  = bs_call_price(bar.close_price, k_call, T, self.risk_free_rate, iv_call)
        p_put   = bs_put_price (bar.close_price, k_put,  T, self.risk_free_rate, iv_put)

        if self.entry_put_short > 0:
            iv_put_short = get_iv(self.codeId, 'put', self.entry_put_short)
            k_put_short  = self._put_strike_by_delta(bar.close_price, T, self.entry_put_short, iv_put_short)
            p_put_short  = bs_put_price(bar.close_price, k_put_short, T, self.risk_free_rate, iv_put_short)
        else:
            iv_put_short = k_put_short = p_put_short = 0.0

        net_debit = p_call + p_put - p_put_short
        self._last_entry_dt = bar.datetime
        self._spreads.append({
            'entry_dt': bar.datetime,
            'k_call': k_call, 'k_put': k_put, 'k_put_short': k_put_short,
            'call_prem': p_call, 'put_prem': p_put, 'put_short_prem': p_put_short,
            'net_debit': net_debit,
            'expiry': expiry,
            'iv_call': iv_call, 'iv_put': iv_put, 'iv_put_short': iv_put_short,
        })
        self._send_order(Direction.LONG, net_debit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓LongStrangle K_call={k_call} K_put={k_put} K_put_short={k_put_short} "
            f"net_debit={net_debit:.2f} expiry={expiry.date()}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._bar_history.append(bar)
        self._check_expiry(bar)
        self._check_entry(bar)
        self._last_bar = bar
