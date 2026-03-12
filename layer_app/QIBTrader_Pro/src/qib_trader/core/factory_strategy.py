from qib_trader.core.engine import BacktestEngine
from qib_trader.strategies.base import Strategy
from qib_trader.core.order_sender import BacktestSender, RealTradeSender, make_order_sender
from qib_trader.utils.config_loader import get_env_config
from qib_trader.strategies import get_strategy_class

class FactoryStrategy:
    @staticmethod
    def create_strategy(strategy_conf: dict, engine: BacktestEngine) -> Strategy:

        # 1. 确定运行模式 [cite: 2026-02-25]
        mode = get_env_config().get("mode", "backtest")

        # 2. 从注册表提取类对象
        class_name = strategy_conf["class"]
        strategy_class = get_strategy_class(class_name)

        if not strategy_class:
            raise ValueError(f"策略类 {class_name} 未在注册表中，请检查 collection 目录")

        # 3. 构造对应的发单器 [cite: 2026-02-25]
        name = strategy_conf["name"]
        symbol = strategy_conf["symbol"]
        sender = make_order_sender(mode, engine, name, symbol)

        # 4. 实例化并注入 (Constructor Injection)
        return strategy_class(
            name=name,
            symbol=symbol,
            interval=strategy_conf["interval"],
            order_sender=sender,  # 注入！ [cite: 2026-02-25]
            direction_limit = strategy_conf["direction_limit"],
            # **strategy_conf.get("params", {})
            params = strategy_conf.get("params", {})
        )