"""
MixMultipleTurn_Long 策略：检测到 mix 型多重底部 UTurn 形态后，
在当前 bar 收盘价开多仓，下一根 bar 收盘价平仓。
"""
from collections import deque

from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData
from qib_trader.utils.bar_patterns import detect_multi_uturn


class MixMultipleTurn_Long(Strategy):
    """
    Mix 型多重底部 UTurn 做多策略（持仓 1 根 bar）。
    params: multi_uturn_window
    """

    multi_uturn_window: int = 300

    def on_init(self) -> None:
        self._bar_history: deque = deque(maxlen=self.multi_uturn_window)
        self._last_bar: BarData | None = None
        self._open_dt = None
        self._open_price: float = 0.0
        self._closed_positions: list[dict] = []

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False
        if self.pos > 0 and self._last_bar is not None:
            self._close_position(self._last_bar.datetime, self._last_bar.close_price)
            self.sell(self._last_bar.close_price, 1, "MKT")

    def _open_position(self, bar: BarData) -> None:
        self._open_dt = bar.datetime
        self._open_price = bar.close_price
        self.buy(bar.close_price, 1, "MKT")

    def _close_position(self, close_dt, close_price: float) -> None:
        self._closed_positions.append({
            'open_dt':    self._open_dt,
            'open_price': self._open_price,
            'close_dt':   close_dt,
            'close_price': close_price,
            'pnl_pts':    close_price - self._open_price,
        })
        self._open_dt = None
        self._open_price = 0.0

    def logic(self, bar: BarData) -> None:
        # 上一根 bar 已开仓 → 本 bar 收盘平仓
        if self.pos > 0:
            self._close_position(bar.datetime, bar.close_price)
            self.sell(bar.close_price, 1, "MKT")

        self._bar_history.append(bar)

        # 无仓位时检测信号
        if self.pos == 0:
            zone = detect_multi_uturn(list(self._bar_history))
            low_breaks_prev = bar.prev_day_low > 0 and bar.low_price < bar.prev_day_low
            if zone is not None and zone.zone_type == 'bottom' and zone.is_mix and low_breaks_prev:
                self._open_position(bar)
                self.logger.info(
                    f"[{bar.datetime}] 开多 close={bar.close_price} "
                    f"uturn_count={zone.uturn_count}"
                )

        self._last_bar = bar
