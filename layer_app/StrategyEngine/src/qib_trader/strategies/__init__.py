import pkgutil
import importlib
import inspect
from typing import Dict, Type

# 导入基类用于类型校验
from .base import Strategy


def discover_strategies() -> Dict[str, Type[Strategy]]:
    """
    自动扫描当前包（strategies/）下的所有模块，
    并提取所有继承自 Strategy 基类的具体实现类。
    """
    strategy_mapping: Dict[str, Type[Strategy]] = {}

    # 1. 遍历当前目录下的所有子模块
    # __path__ 是当前包的物理路径列表
    for loader, module_name, is_pkg in pkgutil.walk_packages(__path__):
        # 排除 base 模块本身，避免循环引用
        if module_name == "base":
            continue

        # 2. 动态导入模块
        # 构造完整路径如 qib_trader.strategies.trend_strategy
        full_module_name = f"{__name__}.{module_name}"
        try:
            module = importlib.import_module(full_module_name)
        except Exception as e:
            # 记录导入失败的模块，但不中断整体程序
            print(f"警告: 无法导入模块 {full_module_name}: {e}")
            continue

        # 3. 检查模块内的所有成员
        for name, obj in inspect.getmembers(module):
            # 筛选标准：是类、继承自 Strategy、不是 Strategy 本身
            if (inspect.isclass(obj) and
                    issubclass(obj, Strategy) and
                    obj is not Strategy):
                # 以类名为 Key 存入映射表
                strategy_mapping[name] = obj

    return strategy_mapping


# --- 统一入口 ---

# 在包初始化时自动执行扫描
STRATEGY_MAP = discover_strategies()


def get_strategy_class(class_name: str) -> Type[Strategy]:
    """
    工厂接口：根据字符串类名获取对应的策略类对象。
    """
    strategy_class = STRATEGY_MAP.get(class_name)
    if not strategy_class:
        available = ", ".join(STRATEGY_MAP.keys())
        raise ImportError(
            f"找不到策略类 '{class_name}'。当前已注册的策略有: [{available}]。 "
            f"请检查配置文件中的 class 字段是否正确，或策略是否继承了基类。"
        )
    return strategy_class