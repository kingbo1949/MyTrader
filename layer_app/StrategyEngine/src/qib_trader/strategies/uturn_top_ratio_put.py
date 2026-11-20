"""
UTurnTopRatioPut 策略：顶部 UTurn 信号 → 开仓 Ratio Put Backspread → 持有至到期结算。
Short 1 put（K1，较高行权价，靠近平值）+ Long 2 puts（K2，较低行权价，更 OTM）。
捕捉顶部 UTurn 后价格继续下跌的空头机会；价格大幅下跌时获利无限。
"""
from collections import deque
from datetime import timedelta, datetime, time as dtime

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing


class UTurnTopRatioPut(Strategy):
    """
    顶部 UTurn Ratio Put Backspread：
    触发顶部 Mix UTurn 信号后，Short 1 put（K1）+ Long 2 puts（K2），持有至到期结算。
    K1 > K2（K1 较高行权价，K2 较低行权价）。
    params: risk_free_rate, option_days, entry_short, entry_long,
            expiry_weekday, cooldown_hours, max_positions,
            require_high_above_prev_day_high, require_plus_pattern
    """

    options_mode: bool = True
    initial_capital: float = 100_000.0

    entry_short: float = 0.5      # short put delta 绝对值（K1，较高行权价，靠近平值）
    entry_long: float = 0.25      # long put delta 绝对值（K2，较低行权价，更 OTM）
    option_days: int = 7
    expiry_weekday: int = -1
    risk_free_rate: float = 0.04
    strike_round: float = 100.0
    multi_uturn_window: int = 300
    cooldown_hours: float = 0.0
    max_positions: int = 0
    require_high_above_prev_day_high: int = 0
    require_plus_pattern: int = 0
    roll_delta_drop: float = 0.0   # short put |delta| 下降超过此值时滚仓（0=不启用）
    long_ratio: int = 2            # long put 手数（≥2）

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
        return datetime.combine(expiry_date, dtime(16, 0))

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
        """到期结算。payoff = 2*max(K2-S,0) - max(K1-S,0)；final_pnl = net_credit + payoff。"""
        S = bar.close_price
        short_settle = max(sp['k1'] - S, 0.0)
        long_settle  = self.long_ratio * max(sp['k2'] - S, 0.0)
        payoff = long_settle - short_settle
        final_pnl = sp['net_credit'] + payoff
        self._send_order(Direction.LONG, sp['net_credit'] - final_pnl, 1, "MKT")
        self._closed_positions.append({
            'entry_dt':      sp['entry_dt'],
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
        self.logger.info(
            f"[{bar.datetime}] TopRatioPut到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} final_pnl={final_pnl:.2f}"
        )

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        """Ratio Put Backspread MTM：Σ (net_credit + ratio×curr_K2_put - curr_K1_put)。"""
        if not self._spreads:
            return 0.0
        S, total = bar.close_price, 0.0
        for sp in self._spreads:
            T       = max((sp['expiry'] - bar.datetime).total_seconds() / (365.25 * 24 * 3600), 1e-8)
            curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            total  += sp['net_credit'] + (self.long_ratio * curr_p2 - curr_p1)
        return total

    # ── 开仓信号 ──────────────────────────────────────────────────────────────

    def _is_plus_pattern(self, zone) -> bool:
        uturn_bars = [b for b in zone.zone_bars
                      if b.div_type in ('Top', 'TopSub') and b.is_uturn]
        if len(uturn_bars) < 2:
            return False
        return abs(uturn_bars[-1].dif) > abs(uturn_bars[-2].dif)

    def _entry_signal(self, bar: BarData) -> bool:
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.max_positions > 0 and len(self._spreads) >= self.max_positions:
            return False
        if self.require_high_above_prev_day_high and bar.prev_day_high > 0 and bar.high_price <= bar.prev_day_high:
            return False
        zone = detect_multi_uturn(list(self._bar_history))
        if zone is None or zone.zone_type != 'top' or not zone.is_mix:
            return False
        if self.require_plus_pattern and not self._is_plus_pattern(zone):
            return False
        return True

    def _open_spread(self, bar: BarData, expiry) -> None:
        """开仓 Ratio Put Backspread：Short 1 put（K1）+ Long 2 puts（K2），K1 > K2。"""
        S = bar.close_price
        T = max((expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600), 1e-8)
        iv_short = get_iv(self.codeId, 'put', self.entry_short)
        iv_long  = get_iv(self.codeId, 'put', self.entry_long)
        k1 = self._put_strike_by_delta(S, T, self.entry_short, iv_short)
        k2 = self._put_strike_by_delta(S, T, self.entry_long,  iv_long)
        p1 = option_pricing.put_price(S, k1, T, self.risk_free_rate, iv_short, self.codeId)
        p2 = option_pricing.put_price(S, k2, T, self.risk_free_rate, iv_long, self.codeId)
        net_credit = p1 - self.long_ratio * p2
        self._spreads.append({
            'entry_dt': bar.datetime, 'k1': k1, 'k2': k2,
            'short_prem': p1, 'long_prem': p2, 'net_credit': net_credit,
            'expiry': expiry, 'iv_short': iv_short, 'iv_long': iv_long,
        })
        self._send_order(Direction.SHORT, net_credit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓TopRatioPutBackspread K1={k1}(short×1) K2={k2}(long×2) "
            f"net_credit={net_credit:.2f} expiry={expiry.date()}"
        )

    def _check_entry(self, bar: BarData) -> None:
        if not self._entry_signal(bar):
            return
        expiry = self._next_expiry(bar.datetime, self.option_days)
        self._last_entry_dt = bar.datetime
        self._open_spread(bar, expiry)

    def _curr_put_delta_abs(self, S: float, sp: dict, bar) -> float:
        """计算仓位 short put 的当前 |delta|。"""
        T = max((sp['expiry'] - bar.datetime).total_seconds() / (365.25 * 24 * 3600), 1e-8)
        return option_pricing.put_delta_abs(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)

    def _roll_spread(self, sp: dict, bar: BarData) -> None:
        """平掉旧仓，在当前价格重新开仓（保持到期日不变）。"""
        S = bar.close_price
        T = max((sp['expiry'] - bar.datetime).total_seconds() / (365.25 * 24 * 3600), 1e-8)
        p1_curr = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
        p2_curr = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
        mark_payoff = self.long_ratio * p2_curr - p1_curr
        roll_pnl = sp['net_credit'] + mark_payoff
        self._send_order(Direction.LONG, sp['net_credit'] - roll_pnl, 1, "MKT")
        self._closed_positions.append({
            'entry_dt': sp['entry_dt'], 'settlement_dt': bar.datetime,
            'close_reason': 'roll', 'k1': sp['k1'], 'k2': sp['k2'],
            'short_prem': sp['short_prem'], 'long_prem': sp['long_prem'],
            'net_credit': sp['net_credit'], 'short_settle': p1_curr,
            'long_settle': self.long_ratio * p2_curr, 'final_pnl': roll_pnl,
        })
        self.logger.info(
            f"[{bar.datetime}] 滚仓 K1={sp['k1']} K2={sp['k2']} "
            f"roll_pnl={roll_pnl:.2f} → 重新开仓 expiry={sp['expiry'].date()}"
        )
        self._open_spread(bar, sp['expiry'])

    def _check_roll(self, bar: BarData) -> None:
        if self.roll_delta_drop <= 0:
            return
        S = bar.close_price
        to_roll, remaining = [], []
        for sp in self._spreads:
            curr_delta = self._curr_put_delta_abs(S, sp, bar)
            if self.entry_short - curr_delta >= self.roll_delta_drop:
                to_roll.append(sp)
            else:
                remaining.append(sp)
        self._spreads = remaining
        for sp in to_roll:
            self._roll_spread(sp, bar)

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._bar_history.append(bar)
        self._check_expiry(bar)
        self._check_roll(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
