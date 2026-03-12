import threading
import logging
from typing import List, Optional

from qib_trader.core.models import BarData, TradeData

logger = logging.getLogger(__name__)

_pool_instance: Optional["StrategyPool"] = None
_pool_lock = threading.Lock()


class StrategyPool:

    def __init__(self):
        self._strategies = []

    def add_strategy(self, strategy) -> None:
        self._strategies.append(strategy)

    def on_init(self) -> None:
        for strategy in self._strategies:
            strategy.on_init()

    def on_start(self) -> None:
        for strategy in self._strategies:
            strategy.on_start()

    def on_stop(self) -> None:
        for strategy in self._strategies:
            strategy.on_stop()

    def on_bar(self, bar: BarData) -> None:
        for strategy in self._strategies:
            strategy.on_bar(bar)

    def on_trade(self, trade: TradeData) -> None:
        for strategy in self._strategies:
            strategy.on_trade(trade)

    def get_strategies(self) -> list:
        """返回策略池中所有策略实例，供结果报告使用。"""
        return list(self._strategies)


def make_pool() -> StrategyPool:
    """创建策略池单例，程序入口调用一次。"""
    global _pool_instance
    with _pool_lock:
        if _pool_instance is None:
            _pool_instance = StrategyPool()
    return _pool_instance


def get_pool() -> StrategyPool:
    """获取策略池单例，未初始化则抛出异常。"""
    if _pool_instance is None:
        raise RuntimeError("StrategyPool 未初始化，请先调用 make_pool()")
    return _pool_instance


def reset_pool() -> None:
    """清空策略池（用于多任务回测之间的清理）"""
    get_pool()._strategies.clear()
