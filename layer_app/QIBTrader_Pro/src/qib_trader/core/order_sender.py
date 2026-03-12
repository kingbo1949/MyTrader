from abc import ABC, abstractmethod
from qib_trader.core.models import Direction

class OrderSender(ABC):
    """发单器抽象接口 (Interface)"""
    @abstractmethod
    def send(self, direction: Direction, price: float, volume: float, order_type: str) -> str:
        pass

class BacktestSender(OrderSender):
    """模拟回测发单实现：发单到模拟柜台"""
    def __init__(self, engine, strategy_name: str, symbol: str):
        self.engine = engine
        self.strategy_name = strategy_name
        self.symbol = symbol

    def send(self, direction: Direction, price: float, volume: float, order_type: str) -> str:
        return self.engine.send_order(
            self.strategy_name, self.symbol, direction, price, volume, order_type
        )

class RealTradeSender(OrderSender):
    """真实柜台发单实现：通过 ICE 调用 C++ 后端接口"""
    def __init__(self, ice_client, strategy_name: str, symbol: str):
        self.ice_client = ice_client
        self.strategy_name = strategy_name
        self.symbol = symbol

    def send(self, direction: Direction, price: float, volume: float, order_type: str) -> str:
        # 调用 ice_client 封装的 C++ 接口 (如 UnifyInterface)
        print(f"[REAL] 策略 {self.strategy_name} 向柜台发送订单")
        return "real_ord_123"


def make_order_sender(mode: str, engine, strategy_name: str, symbol: str) -> OrderSender:
    """
    类厂函数
    """
    if mode == "backtest":
        sender = BacktestSender(engine, strategy_name, symbol)
    else:
        sender = RealTradeSender(engine, strategy_name, symbol)
    return sender

