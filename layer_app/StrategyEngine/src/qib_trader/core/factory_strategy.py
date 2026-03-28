from qib_trader.core.models import Interval, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies import get_strategy_class
from qib_trader.utils.config_loader import get_codeId_config


class FactoryStrategy:
    @staticmethod
    def create_strategy(strategy_conf: dict) -> Strategy:
        class_name = strategy_conf["class"]
        strategy_class = get_strategy_class(class_name)
        params = dict(strategy_conf.get("params", {}))
        # 若 params 未显式指定 strike_round，则从 settings.yaml 注入
        if "strike_round" not in params:
            code_cfg = get_codeId_config(strategy_conf["code_id"])
            if "strike_round" in code_cfg:
                params["strike_round"] = code_cfg["strike_round"]
        return strategy_class(
            codeId=strategy_conf["code_id"],
            interval=Interval(strategy_conf["interval"]),
            direction=Direction(strategy_conf["direction"]),
            params=params
        )
