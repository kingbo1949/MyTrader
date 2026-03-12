from datetime import time
from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction
from qib_trader.core.models import DataUtils


class Break4Week(Strategy):
    """
    N根Bar突破策略。
    开仓限制在 can_open_begin ~ can_open_end 时间区间内；平仓无时间限制。
    LONG：高点突破开多，低点跌破平多。
    SHORT：低点跌破开空，高点突破平空。
    """

    def on_init(self) -> None:
        self.history: list[BarData] = []
        self._open_begin = time.fromisoformat(self.can_open_begin)
        self._open_end = time.fromisoformat(self.can_open_end)
        self.logger.info(
            f"初始化 {self.strategy_id}，bar_count={self.bar_count}，"
            f"开仓窗口={self.can_open_begin}~{self.can_open_end}"
        )

    def on_start(self) -> None:
        self.active = True
        self.logger.info(f"{self.strategy_id} 启动")

    def on_stop(self) -> None:
        self.active = False
        self.logger.info(f"{self.strategy_id} 停止")

    def _can_open(self, bar: BarData) -> bool:
        t = bar.datetime.time()
        return self._open_begin <= t <= self._open_end

    def _highest(self) -> float:
        # DataUtils.print_bars(self.history[-self.bar_count - 1:-1], "highest window")
        return max(b.high_price for b in self.history[-self.bar_count - 1:-1])

    def _lowest(self) -> float:
        return min(b.low_price for b in self.history[-self.bar_count - 1:-1])

    def _try_open_long(self, bar: BarData) -> None:
        highest = self._highest()
        if bar.high_price > highest:
            self.logger.info(f"[{self.strategy_id}] 高点突破 {highest:.2f}，开多")
            self.buy(highest, 1)

    def _try_close_long(self, bar: BarData) -> None:
        lowest = self._lowest()
        if bar.low_price < lowest:
            self.logger.info(f"[{self.strategy_id}] 低点跌破 {lowest:.2f}，平多")
            self.sell(lowest, 1)

    def _try_open_short(self, bar: BarData) -> None:
        lowest = self._lowest()
        if bar.low_price < lowest:
            self.logger.info(f"[{self.strategy_id}] 低点跌破 {lowest:.2f}，开空")
            self.sell(lowest, 1)

    def _try_close_short(self, bar: BarData) -> None:
        highest = self._highest()
        if bar.high_price > highest:
            self.logger.info(f"[{self.strategy_id}] 高点突破 {highest:.2f}，平空")
            self.buy(highest, 1)

    def logic(self, bar: BarData) -> None:
        from datetime import datetime
        # if bar.datetime == datetime(2025, 1, 2, 8, 0, 0):
        #     breakpoint()
        self.history.append(bar)
        if len(self.history) <= self.bar_count:
            return

        if self.direction == Direction.LONG:
            if self.pos == 0 and self._can_open(bar):
                self._try_open_long(bar)
            elif self.pos > 0:
                self._try_close_long(bar)

        elif self.direction == Direction.SHORT:
            if self.pos == 0 and self._can_open(bar):
                self._try_open_short(bar)
            elif self.pos < 0:
                self._try_close_short(bar)
