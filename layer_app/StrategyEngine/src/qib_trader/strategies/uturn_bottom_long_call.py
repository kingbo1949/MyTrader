"""
UTurnBottomLongCall 策略：底部 UTurn 信号 → 买入指定 Delta 的 Call → 防御型锁定 → 到期结算
期权通过 BS 公式模拟定价，所有 PnL 以 点数 记录，multiplier 由 PerformanceAnalyzer 换算 USD。
"""
import math
from datetime import timedelta
from scipy.stats import norm

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction
from qib_trader.utils.config_loader import get_iv


NQ_MULTIPLIER = 20.0  # 每点 $20


def bs_call_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Call 期权 Black-Scholes 定价（T 为年化剩余时间）。"""
    if T <= 0:
        return max(S - K, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


class UTurnBottomLongCall(Strategy):
    """
    底部 UTurn 期权策略（Bull Call Spread），ATM Call 开仓。
    params: iv, risk_free_rate, option_days
    """

    options_mode: bool = True           # 期权 PnL 模式，覆盖基类默认值
    initial_capital: float = 100_000.0  # 覆盖基类默认值（200k）
    min_rise_from_prev_close: float = 0.0  # 盈利锁仓时高价须高于昨收的最小涨幅比率（0=不限制）
    entry_delta: float = 0.5            # 开仓 Call 的目标 Delta（0.5 = ATM）
    stop_loss_ratio: float = 0.6        # 期权价值跌至保费此比例时触发止损
    strike_round: float = 100.0         # 行权价舍入精度（NQ 用 50，ES 用 10，个股用 5）

    def on_init(self) -> None:
        self._long_call: dict | None = None   # 当前持有的 call
        self._spreads: list[dict] = []         # 已锁定 spread，等待到期结算
        self._closed_spreads: list[dict] = []  # 已平仓 spread 的完整生命周期记录
        self._last_bar: BarData | None = None  # 最后一个已处理的 bar（用于强制平仓）

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for sp in self._spreads:
            self._settle_spread(sp, self._last_bar)
        self._spreads.clear()
        self._long_call = None  # 仅有信号尚未锁定 spread，无成交记录，直接丢弃

    # ── 工具方法 ──────────────────────────────────────────────────────────────

    def _strike_by_delta(self, S: float, T: float, iv: float) -> float:
        """由目标 entry_delta 反推行权价，结果取整到最近的 100 点。
        使用简化公式 K = S·exp(-N⁻¹(delta)·σ·√T)，忽略漂移项，
        使 delta=0.5 精确等价于 ATM（K=round(S/100)×100）。"""
        d1 = norm.ppf(self.entry_delta)
        K = S * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
        return round(K / self.strike_round) * self.strike_round

    def _next_expiry(self, bar_dt, min_days: int):
        """返回 bar_dt + min_days 之后的第一个 expiry_weekday（0=周一,2=周三,4=周五）。"""
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

    def _check_early_exit(self, bar: BarData) -> None:
        """锁定 spread 盈利已达最大盈利的 early_exit_ratio，提前平仓。"""
        remaining = []
        for sp in self._spreads:
            rem_t = self._rem_t(bar.datetime, sp['expiry'])
            curr_long  = bs_call_price(bar.close_price, sp['k1'], rem_t, self.risk_free_rate, sp['iv'])
            curr_short = bs_call_price(bar.close_price, sp['k2'], rem_t, self.risk_free_rate, sp['iv'])
            net_debit  = sp['long_prem'] - sp['short_prem']
            max_profit = (sp['k2'] - sp['k1']) - net_debit
            curr_profit = (curr_long - curr_short) - net_debit
            days_left = rem_t * 365.25
            if (self.early_exit_ratio < 1.0
                    and max_profit > 0
                    and days_left >= 1.0
                    and curr_profit >= self.early_exit_ratio * max_profit):
                net_pnl   = curr_profit
                net_debit = sp['long_prem'] - sp['short_prem']
                self._send_order(Direction.SHORT, net_debit + net_pnl,   1, "MKT")
                self._record_close(sp, bar, True, curr_long, curr_short)
                self.logger.info(
                    f"[{bar.datetime}] 提前平仓 K1={sp['k1']} K2={sp['k2']} "
                    f"curr_profit={curr_profit:.2f} max_profit={max_profit:.2f} "
                    f"ratio={curr_profit/max_profit:.1%}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _record_close(self, sp: dict, bar: BarData, is_early_exit: bool,
                      long_settle: float, short_settle: float) -> None:
        """记录 spread 完整生命周期，供 CSV 导出使用。"""
        net_debit = sp['long_prem'] - sp['short_prem']
        payoff = long_settle - short_settle
        self._closed_spreads.append({
            'long_entry_time':         sp['long_entry_time'],
            'short_entry_time':        sp['short_entry_time'],
            'planned_expiry':          sp['expiry'],
            'actual_settlement_time':  bar.datetime,
            'is_early_exit':           is_early_exit,
            'k1':                      sp['k1'],
            'k2':                      sp['k2'],
            'long_entry_price':        sp['long_prem'],
            'short_entry_price':       sp['short_prem'],
            'net_debit':               net_debit,
            'long_settlement_price':   long_settle,
            'short_settlement_price':  short_settle,
            'payoff':                  payoff,
            'net_pnl':                 payoff - net_debit,
        })

    def _settle_spread(self, sp: dict, bar: BarData) -> None:
        """到期结算：只发 SHORT@payoff，LONG 已在锁定时发出。"""
        S = bar.close_price
        long_settle  = max(S - sp['k1'], 0.0)
        short_settle = max(S - sp['k2'], 0.0)
        payoff    = long_settle - short_settle
        net_debit = sp['long_prem'] - sp['short_prem']
        net_pnl   = payoff - net_debit
        self._send_order(Direction.SHORT, net_debit + net_pnl, 1, "MKT")
        self._record_close(sp, bar, False, long_settle, short_settle)
        self.logger.info(
            f"[{bar.datetime}] spread到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} net_pnl_pts={net_pnl:.2f}"
        )

    def _lock_spread(self, bar: BarData, k2: float, lc: dict) -> None:
        """锁定 spread，仅更新内部状态，不产生成交记录（pos 保持 0）。"""
        if k2 <= lc['k1']:
            self.logger.warning(f"[{bar.datetime}] K2({k2:.0f}) <= K1({lc['k1']:.0f})，放弃锁定")
            self._long_call = None
            return
        rem_t = self._rem_t(bar.datetime, lc['expiry'])
        short_prem = bs_call_price(bar.close_price, k2, rem_t, self.risk_free_rate, lc['iv'])
        net_debit = lc['long_prem'] - short_prem
        self._spreads.append({
            'k1': lc['k1'], 'k2': k2,
            'long_prem': lc['long_prem'], 'short_prem': short_prem,
            'expiry': lc['expiry'],
            'long_entry_time': lc['entry_dt'],
            'short_entry_time': bar.datetime,
            'iv': lc['iv'],
        })
        self._send_order(Direction.LONG, net_debit, 1, "MKT")
        self._long_call = None
        self.logger.info(
            f"[{bar.datetime}] 锁定spread K1={lc['k1']} K2={k2:.0f} "
            f"net_debit={(lc['long_prem'] - short_prem):.2f}"
        )

    def _check_timeout(self, bar: BarData) -> None:
        """72h 后按当前价格是否高于开仓价决定 K2。"""
        if self._long_call is None:
            return
        lc = self._long_call
        if bar.datetime < lc['entry_dt'] + timedelta(hours=72):
            return
        k2 = (bar.close_price + self.k2_timeout_profit) if bar.close_price > lc['entry_close'] \
             else (lc['entry_close'] + self.k2_timeout_loss)
        self._lock_spread(bar, k2, lc)

    def _check_stop_loss(self, bar: BarData) -> None:
        """当前期权价值 ≤ 60% 保费，转为 spread 止损。"""
        if self._long_call is None:
            return
        lc = self._long_call
        rem_t = self._rem_t(bar.datetime, lc['expiry'])
        curr_val = bs_call_price(bar.close_price, lc['k1'], rem_t, self.risk_free_rate, lc['iv'])
        if curr_val <= self.stop_loss_ratio * lc['long_prem']:
            self._lock_spread(bar, lc['entry_close'] + self.k2_stoploss, lc)

    def _check_take_profit(self, bar: BarData) -> None:
        """顶部 UTurn 信号且价格创新高，转为 spread 止盈。"""
        if self._long_call is None:
            return
        is_top = bar.div_type in ('Top', 'TopSub') and bar.is_uturn
        if (is_top and bar.dif > self.dif_profit and bar.prev_day_high > 0
                and bar.high_price > bar.prev_day_high
                and (bar.prev_day_close <= 0
                     or bar.high_price > bar.prev_day_close * (1 + self.min_rise_from_prev_close))):
            self._lock_spread(bar, bar.close_price + self.k2_takeprofit, self._long_call)

    def _entry_signal(self, bar: BarData) -> bool:
        """底部 UTurn + dif < dif_entry + low 破昨低 + low 低于昨收 min_drop_from_prev_close。"""
        is_bottom = bar.div_type in ('Bottom', 'BottomSub') and bar.is_uturn
        low_below_prev_close = (bar.prev_day_close <= 0
                                or bar.low_price < bar.prev_day_close * (1 - self.min_drop_from_prev_close))
        return (is_bottom
                and bar.dif < self.dif_entry
                and bar.prev_day_low > 0
                and bar.low_price < bar.prev_day_low
                and low_below_prev_close)

    def _check_entry(self, bar: BarData) -> None:
        """买入 ATM call，开仓条件由 _entry_signal 决定。"""
        if self._long_call is not None:
            return
        if not self._entry_signal(bar):
            return
        expiry = self._next_expiry(bar.datetime, self.option_days)
        T = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv_entry = get_iv(self.codeId, 'call', self.entry_delta)
        k1 = self._strike_by_delta(bar.close_price, T, iv_entry)
        long_prem = bs_call_price(bar.close_price, k1, T, self.risk_free_rate, iv_entry)
        self._long_call = {
            'entry_dt': bar.datetime, 'entry_close': bar.close_price,
            'k1': k1, 'expiry': expiry, 'long_prem': long_prem, 'iv': iv_entry,
        }
        self.logger.info(
            f"[{bar.datetime}] 底部UTurn开仓 K1={k1} prem={long_prem:.2f} "
            f"expiry={expiry.date()} dif={bar.dif:.1f}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_expiry(bar)
        self._check_early_exit(bar)
        self._check_timeout(bar)
        self._check_stop_loss(bar)
        self._check_take_profit(bar)
        self._check_entry(bar)
        self._last_bar = bar
