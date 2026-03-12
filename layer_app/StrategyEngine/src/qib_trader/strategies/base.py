import uuid
from abc import ABC, abstractmethod
from datetime import datetime
from typing import Dict, Any
import logging

from qib_trader.core.models import BarData, Direction, OrderData, TradeData, Interval, CodeId, StrategyId, OrderId


class Strategy(ABC):

    def __init__(self, codeId: CodeId, interval: Interval, direction: Direction, params: Dict[str, Any]):
        self.codeId: CodeId = codeId
        self.interval: Interval = interval
        self.direction: Direction = direction
        self.strategy_id: StrategyId = f"{type(self).__name__}_{codeId}_{interval.value}_{direction.value}"

        self.params: Dict[str, Any] = params
        self.active: bool = False
        self.pos: float = 0.0
        self._current_dt: datetime = datetime.min

        self.logger = logging.getLogger(f"Strategy.{self.strategy_id}")
        self._update_params(params)

    def _update_params(self, params: Dict[str, Any]) -> None:
        for key, value in params.items():
            setattr(self, key, value)

    # --- 生命周期钩子 ---

    @abstractmethod
    def on_init(self) -> None:
        pass

    @abstractmethod
    def on_start(self) -> None:
        self.active = True
        self.logger.info(f"策略 {self.strategy_id} 已启动")

    @abstractmethod
    def on_stop(self) -> None:
        self.active = False
        self.logger.info(f"策略 {self.strategy_id} 已停止")

    # --- 数据驱动钩子 ---

    def on_bar(self, bar: BarData) -> None:
        if bar.codeId != self.codeId:
            return
        if bar.interval != self.interval:
            return
        self._current_dt = bar.datetime
        self.logic(bar)

    @abstractmethod
    def logic(self, bar: BarData) -> None:
        pass

    # --- 交易反馈钩子 ---

    def on_order(self, order: OrderData) -> None:
        pass

    def on_trade(self, trade: TradeData) -> None:
        if trade.strategy_id != self.strategy_id:
            return
        match trade.direction:
            case Direction.LONG:
                self.pos += trade.volume
            case Direction.SHORT:
                self.pos -= trade.volume
        self.logger.info(f"持仓更新: {self.pos} | 成交: {trade.direction} {trade.volume}")

    # --- 交易指令接口 ---

    def buy(self, price: float, volume: float, order_type: str = "LMT") -> None:
        if self.direction == Direction.SHORT and self.pos >= 0:
            self.logger.debug(f"{self.strategy_id}: SHORT 策略拒绝开多")
            return
        self._send_order(Direction.LONG, price, volume, order_type)

    def sell(self, price: float, volume: float, order_type: str = "LMT") -> None:
        if self.direction == Direction.LONG and self.pos <= 0:
            self.logger.debug(f"{self.strategy_id}: LONG 策略拒绝开空")
            return
        self._send_order(Direction.SHORT, price, volume, order_type)

    def _send_order(self, direction: Direction, price: float, volume: float, order_type: str) -> None:
        from qib_trader.core.broker import get_broker
        order = OrderData(
            order_id=f"ord_{uuid.uuid4().hex[:8]}",
            strategy_id=self.strategy_id,
            codeId=self.codeId,
            direction=direction,
            price=price,
            volume=volume,
            datetime=self._current_dt
        )
        get_broker().on_order(order)
