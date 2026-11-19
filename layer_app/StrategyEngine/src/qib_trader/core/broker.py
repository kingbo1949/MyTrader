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
    实盘自动下单柜台：Interactive Brokers API 封装（接口预留，暂未实现）。
    对应 settings.yaml 里 mode="real"——真正会调用 ib_insync 下单接口，
    在自动下单阶段真正实现之前，任何调用都应该直接失败，不能默默什么都不做。
    """

    def reset(self) -> None:
        pass

    def on_order(self, order: OrderData) -> None:
        raise NotImplementedError

    def on_bar(self, bar: BarData) -> None:
        pass  # 实盘行情由 IB 主动推送，无需此入口

    def get_trades(self) -> TradeDatas:
        raise NotImplementedError


class LivePrintBroker(Broker):
    """
    实盘信号柜台（Phase 1：只打印/展示不下单）。
    对应 settings.yaml 里 mode="live_signal"——刻意用一个跟 "real"（IBBroker，
    预留给未来真正自动下单）不同的字符串，避免两个阶段的含义被混用、
    误触发下单。

    行为上跟 SimBroker 一样按委托价直接"记账"（保证策略内部 _closed_positions/
    统计逻辑不受影响），但完全不 import ib_insync——结构上保证这个阶段的
    代码路径不可能触发真实下单。真正的"建议下单"信息走的是另一条独立通道
    （strategies 里的 emit_intent() → live/intent.py 的 sink），不经过这里。
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
        self._trades.append(trade)
        get_pool().on_trade(trade)
        logger.debug(f"[LivePrintBroker] 仅记账，未下单: {trade.trade_id} | {trade.direction} {trade.volume} @ {trade.price}")

    def on_bar(self, bar: BarData) -> None:
        pass

    def get_trades(self) -> TradeDatas:
        return list(self._trades)


_broker_instance: Optional[Broker] = None
_broker_lock = threading.Lock()


def make_broker() -> Broker:
    """创建柜台单例，程序入口调用一次。mode: backtest→SimBroker；
    live_signal→LivePrintBroker（只记账不下单）；其余（real）→IBBroker（预留，未实现）。"""
    global _broker_instance
    with _broker_lock:
        if _broker_instance is None:
            mode = get_env_config().get("mode", "backtest")
            if mode == "backtest":
                _broker_instance = SimBroker()
            elif mode == "live_signal":
                _broker_instance = LivePrintBroker()
            else:
                _broker_instance = IBBroker()
    return _broker_instance


def get_broker() -> Broker:
    """获取柜台单例，未初始化则抛出异常。"""
    if _broker_instance is None:
        raise RuntimeError("Broker 未初始化，请先调用 make_broker()")
    return _broker_instance


def reset_broker() -> None:
    """重置柜台状态（用于多任务回测之间的清理）"""
    get_broker().reset()
