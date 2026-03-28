"""
UTurnBottomShortPut 策略：底部 UTurn 信号 → 直接开仓 Bull Put Spread → 持有至到期结算。
同时卖出 short put（K1，较高行权价）和买入 long put（K2，较低行权价），收取净权利金。
"""
import math
from collections import deque
from datetime import timedelta
from scipy.stats import norm

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import get_iv


def bs_put_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Put 期权 Black-Scholes 定价（T 为年化剩余时间）。"""
    if T <= 0:
        return max(K - S, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return K * math.exp(-r * T) * norm.cdf(-d2) - S * norm.cdf(-d1)


class UTurnBottomShortPut(Strategy):
    """
    底部 UTurn 期权策略（Bull Put Spread），触发信号后立即同时开两腿，持有至到期结算。
    params: iv, risk_free_rate, option_days, entry_short, entry_long,
            expiry_weekday, min_drop_from_prev_close, cooldown_hours
    """

    options_mode: bool = True
    initial_capital: float = 100_000.0

    # 参数默认值（会被 params 覆盖）
    entry_short: float = 0.3          # 卖出腿 put delta 绝对值（K1，较高行权价）
    entry_long: float = 0.1           # 买入腿 put delta 绝对值（K2，较低行权价）
    option_days: int = 7              # 期权最短到期天数
    expiry_weekday: int = 2           # 到期日星期几（0=周一...4=周五）
    risk_free_rate: float = 0.04      # 无风险利率
    strike_round: float = 100.0       # 行权价舍入精度（NQ/ES 用 100，个股如 TSLA 用 5）
    min_drop_from_prev_close: float = 0.018  # 开仓低价须低于昨收的最小跌幅
    multi_uturn_window: int = 50      # 多重 uturn 检测的 bar 历史窗口大小
    cooldown_hours: float = 0.0       # 开仓后冷却时间（小时），期间不允许再次开仓
    require_low_below_prev_day_low: int = 0  # 1=开仓bar最低价必须小于上一日最低价；0=不限制

    def on_init(self) -> None:
        self._spreads: list[dict] = []         # 持有中的 bull put spread，等待到期
        self._closed_positions: list[dict] = []  # 已平仓记录
        self._last_bar: BarData | None = None
        self._last_entry_dt = None             # 最近一次开仓时间
        self._bar_history: deque = deque(maxlen=self.multi_uturn_window)

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for sp in self._spreads:
            self._settle_spread(sp, self._last_bar)
        self._spreads.clear()

    # ── 工具方法 ──────────────────────────────────────────────────────────────

    def _put_strike_by_delta(self, S: float, T: float, put_delta_abs: float, iv: float) -> float:
        """由 put delta 绝对值反推行权价，取整到最近的 strike_round 点。"""
        d1 = norm.ppf(1.0 - put_delta_abs)
        K = S * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
        return round(K / self.strike_round) * self.strike_round

    def _next_expiry(self, bar_dt, min_days: int):
        """返回 bar_dt + min_days 之后的第一个 expiry_weekday。"""
        target = bar_dt + timedelta(days=min_days)
        days_ahead = (self.expiry_weekday - target.weekday()) % 7
        return target + timedelta(days=days_ahead)

    def _rem_t(self, bar_dt, expiry) -> float:
        delta = (expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600)
        return max(delta, 0.0)

    # ── 持仓管理 ──────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        """到期的 spread 结算，未到期的保留。"""
        remaining = []
        for sp in self._spreads:
            if bar.datetime >= sp['expiry']:
                self._settle_spread(sp, bar)
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _settle_spread(self, sp: dict, bar: BarData) -> None:
        """到期结算。payoff = max(K2-S,0) - max(K1-S,0)；final_pnl = net_credit + payoff。"""
        S = bar.close_price
        short_settle = max(sp['k1'] - S, 0.0)
        long_settle  = max(sp['k2'] - S, 0.0)
        payoff = long_settle - short_settle
        net_credit = sp['net_credit']
        final_pnl = net_credit + payoff
        self._send_order(Direction.LONG, net_credit - final_pnl, 1, "MKT")
        self._record_position(sp, bar, 'expiry', short_settle, long_settle, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] spread到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} final_pnl={final_pnl:.2f}"
        )

    def _record_position(self, sp: dict, bar: BarData, close_reason: str,
                         short_settle: float, long_settle: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':      sp['entry_dt'],
            'lock_dt':       sp.get('lock_dt'),
            'lock_reason':   sp.get('lock_reason'),
            'settlement_dt': bar.datetime,
            'close_reason':  close_reason,
            'k1':            sp['k1'],
            'k2':            sp['k2'],
            'short_prem':    sp['short_prem'],
            'long_prem':     sp['long_prem'],
            'net_credit':    sp['net_credit'],
            'short_settle':  short_settle,
            'long_settle':   long_settle,
            'final_pnl':     final_pnl,
        })

    def _entry_signal(self, bar: BarData) -> bool:
        """仅允许 mix 型多重底部 uturn 开仓（区间内存在至少一根 macd > 0 的 bar）。"""
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.require_low_below_prev_day_low and bar.prev_day_low > 0 and bar.low_price >= bar.prev_day_low:
            return False
        zone = detect_multi_uturn(list(self._bar_history))
        return zone is not None and zone.zone_type == 'bottom' and zone.is_mix

    def _check_entry(self, bar: BarData) -> None:
        """直接开仓 Bull Put Spread：同时卖出 K1 put 和买入 K2 put。"""
        if not self._entry_signal(bar):
            return
        expiry = self._next_expiry(bar.datetime, self.option_days)
        T = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv_short = get_iv(self.codeId, 'put', self.entry_short)
        iv_long  = get_iv(self.codeId, 'put', self.entry_long)
        k1 = self._put_strike_by_delta(bar.close_price, T, self.entry_short, iv_short)
        k2 = self._put_strike_by_delta(bar.close_price, T, self.entry_long,  iv_long)
        p1 = bs_put_price(bar.close_price, k1, T, self.risk_free_rate, iv_short)
        p2 = bs_put_price(bar.close_price, k2, T, self.risk_free_rate, iv_long)
        net_credit = p1 - p2
        self._last_entry_dt = bar.datetime
        self._spreads.append({
            'entry_dt': bar.datetime, 'lock_dt': bar.datetime, 'lock_reason': 'direct_entry',
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
        self._check_entry(bar)
        self._last_bar = bar
