"""
UTurnBottom 策略：底部 UTurn 信号 → 买入 ATM Call → 防御型锁定 → 到期结算
期权通过 BS 公式模拟定价，所有 PnL 以 点数 记录，multiplier 由 PerformanceAnalyzer 换算 USD。
"""
import math
from datetime import timedelta
from scipy.stats import norm

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction


NQ_MULTIPLIER = 20.0  # 每点 $20


def bs_call_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Call 期权 Black-Scholes 定价（T 为年化剩余时间）。"""
    if T <= 0:
        return max(S - K, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


class UTurnBottom(Strategy):
    """
    底部 UTurn 期权策略（Bull Call Spread）。
    params: iv, risk_free_rate, option_days
    """

    def on_init(self) -> None:
        self._long_call: dict | None = None   # 当前持有的 call
        self._spreads: list[dict] = []         # 已锁定 spread，等待到期结算

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False

    # ── 工具方法 ──────────────────────────────────────────────────────────────

    def _atm_strike(self, close: float) -> float:
        return round(close / 100.0) * 100.0

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
        """计算 spread 到期 PnL，记录合成 TradeData。"""
        S = bar.close_price
        payoff = max(S - sp['k1'], 0.0) - max(S - sp['k2'], 0.0)
        # settlement_price 使 (settlement - long_prem) * mult = 真实 PnL
        settlement = payoff + sp['short_prem']
        self._send_order(Direction.SHORT, settlement, 1, "MKT")
        pnl_pts = payoff - (sp['long_prem'] - sp['short_prem'])
        self.logger.info(
            f"[{bar.datetime}] spread到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} net_pnl_pts={pnl_pts:.2f}"
        )

    def _lock_spread(self, bar: BarData, k2: float, lc: dict) -> None:
        """将 long_call 转为 bull call spread，释放开仓名额。"""
        rem_t = self._rem_t(bar.datetime, lc['expiry'])
        short_prem = bs_call_price(bar.close_price, k2, rem_t, self.risk_free_rate, self.iv)
        self._spreads.append({
            'k1': lc['k1'], 'k2': k2,
            'long_prem': lc['long_prem'], 'short_prem': short_prem,
            'expiry': lc['expiry'],
        })
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
        k2 = (bar.close_price + 100.0) if bar.close_price > lc['entry_close'] \
             else (lc['entry_close'] + 100.0)
        self._lock_spread(bar, k2, lc)

    def _check_stop_loss(self, bar: BarData) -> None:
        """当前期权价值 ≤ 50% 保费，转为 spread 止损。"""
        if self._long_call is None:
            return
        lc = self._long_call
        rem_t = self._rem_t(bar.datetime, lc['expiry'])
        curr_val = bs_call_price(bar.close_price, lc['k1'], rem_t, self.risk_free_rate, self.iv)
        if curr_val <= 0.5 * lc['long_prem']:
            self._lock_spread(bar, lc['entry_close'] + 100.0, lc)

    def _check_take_profit(self, bar: BarData) -> None:
        """顶部 UTurn 信号且价格创新高，转为 spread 止盈。"""
        if self._long_call is None:
            return
        is_top = bar.div_type in ('Top', 'TopSub') and bar.is_uturn
        if is_top and bar.dif > 30 and bar.prev_day_high > 0 and bar.high_price > bar.prev_day_high:
            self._lock_spread(bar, bar.close_price + 100.0, self._long_call)

    def _check_entry(self, bar: BarData) -> None:
        """底部 UTurn + dif < -30 + low < 昨低，买入 ATM call。"""
        if self._long_call is not None:
            return
        is_bottom = bar.div_type in ('Bottom', 'BottomSub') and bar.is_uturn
        if not (is_bottom and bar.dif < -30 and bar.prev_day_low > 0
                and bar.low_price < bar.prev_day_low):
            return
        k1 = self._atm_strike(bar.close_price)
        T = self.option_days / 365.25
        long_prem = bs_call_price(bar.close_price, k1, T, self.risk_free_rate, self.iv)
        expiry = bar.datetime + timedelta(days=self.option_days)
        self._long_call = {
            'entry_dt': bar.datetime, 'entry_close': bar.close_price,
            'k1': k1, 'expiry': expiry, 'long_prem': long_prem,
        }
        # 记录开仓：buy at long_prem (points)，PnL 由 settle 时的 SHORT 配对
        self._send_order(Direction.LONG, long_prem, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 底部UTurn开仓 K1={k1} prem={long_prem:.2f} "
            f"expiry={expiry.date()} dif={bar.dif:.1f}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_expiry(bar)
        self._check_timeout(bar)
        self._check_stop_loss(bar)
        self._check_take_profit(bar)
        self._check_entry(bar)
