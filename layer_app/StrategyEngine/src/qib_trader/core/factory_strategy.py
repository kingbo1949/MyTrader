from qib_trader.core.models import Interval, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies import get_strategy_class


class FactoryStrategy:
    @staticmethod
    def create_strategy(strategy_conf: dict) -> Strategy:
        class_name = strategy_conf["class"]
        strategy_class = get_strategy_class(class_name)

        return strategy_class(
            codeId=strategy_conf["code_id"],
            interval=Interval(strategy_conf["interval"]),
            direction=Direction(strategy_conf["direction"]),
            params=strategy_conf.get("params", {})
        )
