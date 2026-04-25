"""
UTurnLongCallMomentum 策略：底部 Mix UTurn 信号 → 买入近 ATM Call → 主动 MtM 止盈/止损。

与 UTurnBottomLongCall（Bull Spread）的核心区别：
1. 单腿 Long Call，上行收益无上限
2. 每根 bar 检查期权市值，触达 take_profit_ratio 或 stop_loss_ratio 立即平仓
3. 仓位大小按 risk_per_trade × initial_capital 动态计算手数

正期望值依据：
- UTurn Bottom 后短期均值回归倾向较强，ATM Call 的 gamma 最大，趋势延续时价值增速快
- 止损 50% + 止盈 100%：胜率 >33% 即可盈利
- 每笔风险 1% → 连续 10 笔全损才触达 10% 最大回撤
"""
import math
from collections import deque
from datetime import timedelta, datetime, time as dtime
from scipy.stats import norm

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import get_iv, get_codeId_config


def bs_call_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Call 期权 Black-Scholes 定价（T 为年化剩余时间）。"""
    if T <= 0:
        return max(S - K, 0.0)
    if K <= 0 or sigma <= 0:
        return max(S - K, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


class UTurnLongCallMomentum(Strategy):
    """
    底部 UTurn 买方动量策略：买入 ATM Call，主动监控 MtM，触达止盈/止损即平仓。
    params: entry_delta, option_days, expiry_weekday, risk_free_rate,
            take_profit_ratio, stop_loss_ratio, risk_per_trade,
            cooldown_hours, max_positions, require_low_below_prev_day_low
    """

    options_mode: bool = True
    initial_capital: float = 100_000.0

    entry_delta: float = 0.5           # 买入 Call 的目标 delta（0.5 = ATM）
    option_days: int = 7
    expiry_weekday: int = -1           # -1=每日期权；0~4=指定星期几
    risk_free_rate: float = 0.04
    strike_round: float = 100.0

    take_profit_ratio: float = 1.0     # 市值涨幅达到权利金的此倍数时止盈（1.0 = 100% 收益）
    stop_loss_ratio: float = 0.5       # 市值跌幅达到权利金的此比例时止损（0.5 = 50% 亏损）
    risk_per_trade: float = 0.01       # 每笔风险占 initial_capital 的比例（0.01 = 1%）

    multi_uturn_window: int = 300
    cooldown_hours: float = 0.0
    max_positions: int = 0
    require_low_below_prev_day_low: int = 0
    min_drop_from_prev_close: float = 0.0

    def on_init(self) -> None:
        self._positions: list[dict] = []
        self._closed_positions: list[dict] = []
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._bar_history: deque = deque(maxlen=self.multi_uturn_window)
        self._multiplier: float = get_codeId_config(self.codeId).get('multiplier', 1.0)

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for pos in self._positions:
            self._close_position(pos, self._last_bar, 'force_close')
        self._positions.clear()

    # ── 工具方法 ──────────────────────────────────────────────────────────────

    def _call_strike_by_delta(self, S: float, T: float, call_delta: float, iv: float) -> float:
        d1 = norm.ppf(call_delta)
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
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    def _calc_volume(self, entry_prem: float) -> int:
        """按风险额度计算手数：risk_amount / (entry_prem × stop_loss_ratio × multiplier)，最少 1 手。"""
        risk_amount = self.initial_capital * self.risk_per_trade
        loss_per_lot = entry_prem * self.stop_loss_ratio * self._multiplier
        if loss_per_lot <= 0:
            return 1
        return max(1, int(risk_amount / loss_per_lot))

    # ── 持仓管理 ──────────────────────────────────────────────────────────────

    def _curr_value(self, pos: dict, bar: BarData) -> float:
        T = self._rem_t(bar.datetime, pos['expiry'])
        return bs_call_price(bar.close_price, pos['k'], T, self.risk_free_rate, pos['iv'])

    def _close_position(self, pos: dict, bar: BarData, close_reason: str) -> None:
        if close_reason == 'expiry':
            exit_prem = max(bar.close_price - pos['k'], 0.0)
        else:
            exit_prem = self._curr_value(pos, bar)
        final_pnl = (exit_prem - pos['entry_prem']) * pos['volume']
        self._send_order(Direction.SHORT, exit_prem, pos['volume'], "MKT")
        self._closed_positions.append({
            'entry_dt':      pos['entry_dt'],
            'settlement_dt': bar.datetime,
            'close_reason':  close_reason,
            'k':             pos['k'],
            'entry_prem':    pos['entry_prem'],
            'exit_prem':     exit_prem,
            'volume':        pos['volume'],
            'final_pnl':     final_pnl,
        })
        self.logger.info(
            f"[{bar.datetime}] {close_reason} K={pos['k']} "
            f"entry={pos['entry_prem']:.2f} exit={exit_prem:.2f} "
            f"pnl={final_pnl:.2f} vol={pos['volume']}"
        )

    def _check_expiry(self, bar: BarData) -> None:
        remaining = []
        for pos in self._positions:
            if bar.datetime >= pos['expiry']:
                self._close_position(pos, bar, 'expiry')
            else:
                remaining.append(pos)
        self._positions = remaining

    def _check_exit(self, bar: BarData) -> None:
        """每根 bar 检查止盈/止损条件。"""
        remaining = []
        for pos in self._positions:
            curr = self._curr_value(pos, bar)
            gain = (curr - pos['entry_prem']) / pos['entry_prem']
            if gain >= self.take_profit_ratio:
                self._close_position(pos, bar, 'take_profit')
            elif gain <= -self.stop_loss_ratio:
                self._close_position(pos, bar, 'stop_loss')
            else:
                remaining.append(pos)
        self._positions = remaining

    # ── 开仓信号 ──────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.max_positions > 0 and len(self._positions) >= self.max_positions:
            return False
        if self.require_low_below_prev_day_low and bar.prev_day_low > 0 and bar.low_price >= bar.prev_day_low:
            return False
        zone = detect_multi_uturn(list(self._bar_history))
        return zone is not None and zone.zone_type == 'bottom' and zone.is_mix

    def _check_entry(self, bar: BarData) -> None:
        if not self._entry_signal(bar):
            return
        expiry = self._next_expiry(bar.datetime, self.option_days)
        T = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv = get_iv(self.codeId, 'call', self.entry_delta)
        k = self._call_strike_by_delta(bar.close_price, T, self.entry_delta, iv)
        entry_prem = bs_call_price(bar.close_price, k, T, self.risk_free_rate, iv)
        volume = self._calc_volume(entry_prem)
        self._last_entry_dt = bar.datetime
        self._positions.append({
            'entry_dt':   bar.datetime,
            'k':          k,
            'entry_prem': entry_prem,
            'volume':     volume,
            'expiry':     expiry,
            'iv':         iv,
        })
        self._send_order(Direction.LONG, entry_prem, volume, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓LongCall K={k} prem={entry_prem:.2f} "
            f"vol={volume} expiry={expiry.date()} "
            f"TP={self.take_profit_ratio*100:.0f}% SL={self.stop_loss_ratio*100:.0f}%"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._bar_history.append(bar)
        self._check_expiry(bar)
        self._check_exit(bar)
        self._check_entry(bar)
        self._last_bar = bar
