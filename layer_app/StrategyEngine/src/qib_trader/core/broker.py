import uuid
import threading
import logging
from abc import ABC, abstractmethod
from typing import Optional

from qib_trader.core.models import BarData, OrderData, TradeData, TradeDatas
from qib_trader.utils.config_loader import get_env_config

logger = logging.getLogger(__name__)


class Broker(ABC):
    """
    柜台抽象基类。
    定义回测与实盘共同的最小接口。
    """

    @abstractmethod
    def reset(self) -> None:
        """重置状态，用于多任务回测之间的清理"""
        ...

    @abstractmethod
    def on_order(self, order: OrderData) -> None:
        """接收委托"""
        ...

    @abstractmethod
    def on_bar(self, bar: BarData) -> None:
        """
        推入行情。
        回测模式：触发撮合；实盘模式：空实现（行情由 IB 主动推送）。
        """
        ...

    @abstractmethod
    def get_trades(self) -> TradeDatas:
        """返回全部成交记录，供统计模块使用"""
        ...


class SimBroker(Broker):
    """
    模拟回测柜台。
    撮合规则：收到委托后按委托价直接成交。
    """

    def __init__(self):
        self._trades: TradeDatas = []

    def reset(self) -> None:
        self._trades.clear()

    def on_order(self, order: OrderData) -> None:
        from qib_trader.core.strategy_pool import get_pool
        trade = TradeData(
            codeId=order.codeId,
            order_id=order.order_id,
            trade_id=f"trd_{uuid.uuid4().hex[:8]}",
            strategy_id=order.strategy_id,
            direction=order.direction,
            price=order.price,
            volume=order.volume,
            datetime=order.datetime
        )
        # 按下单价格直接撮合成交
        self._trades.append(trade)
        get_pool().on_trade(trade)
        logger.debug(f"成交: {trade.trade_id} | {trade.direction} {trade.volume} @ {trade.price}")

    def on_bar(self, bar: BarData) -> None:
        pass

    def get_trades(self) -> TradeDatas:
        return list(self._trades)


class IBBroker(Broker):
    """
    实盘柜台：Interactive Brokers API 封装（接口预留，暂未实现）。
    """

    def reset(self) -> None:
        pass

    def on_order(self, order: OrderData) -> None:
        raise NotImplementedError

    def on_bar(self, bar: BarData) -> None:
        pass  # 实盘行情由 IB 主动推送，无需此入口

    def get_trades(self) -> TradeDatas:
        raise NotImplementedError


_broker_instance: Optional[Broker] = None
_broker_lock = threading.Lock()


def make_broker() -> Broker:
    """创建柜台单例，程序入口调用一次。"""
    global _broker_instance
    with _broker_lock:
        if _broker_instance is None:
            mode = get_env_config().get("mode", "backtest")
            _broker_instance = SimBroker() if mode == "backtest" else IBBroker()
    return _broker_instance


def get_broker() -> Broker:
    """获取柜台单例，未初始化则抛出异常。"""
    if _broker_instance is None:
        raise RuntimeError("Broker 未初始化，请先调用 make_broker()")
    return _broker_instance


def reset_broker() -> None:
    """重置柜台状态（用于多任务回测之间的清理）"""
    get_broker().reset()
