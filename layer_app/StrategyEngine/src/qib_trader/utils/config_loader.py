import yaml
from pathlib import Path
from typing import Dict, Any
from qib_trader.core.models import CodeId


def get_project_root() -> Path:
    """
    获取项目根目录 QIBTrader_Pro/
    基于当前文件位置 (src/qib_trader/utils/config_loader.py) 向上追溯三级。
    """
    return Path(__file__).resolve().parents[3]


def load_yaml_config(file_path: str) -> Dict[str, Any]:
    """
    加载并解析 YAML 配置文件。
    [保持] 自动定位根目录逻辑，确保相对路径在不同执行环境下均有效。
    """
    # 1. 拼接目标配置文件的绝对路径
    absolute_path = get_project_root() / file_path

    if not absolute_path.exists():
        # [保持] 报错时输出绝对路径，方便调试
        raise FileNotFoundError(f"无法找到配置文件。请确认文件存在于: {absolute_path}")

    with open(absolute_path, 'r', encoding='utf-8') as f:
        # [保持] 使用 safe_load 确保解析安全
        config = yaml.safe_load(f)

    return config if config else {}


def get_codeId_config(codeId: CodeId) -> Dict[str, Any]:
    """
    [新增] 从 config/settings.yaml 中获取特定品种的全局配置属性。
    用于动态获取合约乘数 (Multiplier) 和手续费 (Commission)。
    """
    try:
        # 默认读取 settings.yaml
        settings = load_yaml_config("config/settings.yaml")
        codeIds_section = settings.get("codeIds", {})

        # 获取特定品种配置，若不存在则返回默认乘数 1.0
        symbol_data = codeIds_section.get(codeId)

        if not symbol_data:
            # 此处建议记录警告日志，提醒该品种未在 settings.yaml 中定义
            return {"multiplier": 1.0, "commission": 0.0}

        return symbol_data

    except FileNotFoundError:
        # 如果 settings.yaml 不存在，返回保守默认值
        return {"multiplier": 1.0, "commission": 0.0}

def get_iv(codeId: str, option_type: str, delta: float) -> float:
    """从 settings.yaml 按品种+期权类型+delta 查找 IV。
    精确匹配直接返回；多档位时线性插值/外推；单档位时返回该值；表不存在返回 0.24。
    """
    iv_table = get_codeId_config(codeId).get("iv", {}).get(option_type, {})
    if not iv_table:
        return 0.24
    key = round(delta, 2)
    if key in iv_table:
        return iv_table[key]
    if len(iv_table) == 1:
        return next(iter(iv_table.values()))
    deltas = sorted(iv_table.keys())
    # 找到相邻两个档位做线性插值（含端点外推）
    for i in range(len(deltas) - 1):
        d0, d1 = deltas[i], deltas[i + 1]
        if d0 <= delta <= d1:
            t = (delta - d0) / (d1 - d0)
            return iv_table[d0] + t * (iv_table[d1] - iv_table[d0])
    # 超出范围：用最近两端做线性外推
    if delta < deltas[0]:
        d0, d1 = deltas[0], deltas[1]
    else:
        d0, d1 = deltas[-2], deltas[-1]
    t = (delta - d0) / (d1 - d0)
    return iv_table[d0] + t * (iv_table[d1] - iv_table[d0])


def get_env_config() -> Dict[str, Any]:
    """
    从 config/settings.yaml 中获取环境的全局配置属性。
    """
    settings = load_yaml_config("config/settings.yaml")
    env_section = settings.get("environment", {})
    return env_section


def is_export_trades_csv() -> bool:
    """返回是否在回测结束后导出成交记录 CSV。"""
    return bool(get_env_config().get("export_trades_csv", False))

def get_strategies_config() -> [Any]:
    """
    从 config/strategies.yaml 中获取环境的全局配置属性。
    """
    loader = load_yaml_config("config/strategies.yaml")
    strategy_configs = loader.get("strategies", [])
    return strategy_configs


def get_wfo_config() -> [Any]:
    """从 config/wfo.yaml 中获取 Walk-Forward Optimization 任务配置。"""
    loader = load_yaml_config("config/wfo.yaml")
    return loader.get("wfo_tasks", [])


def get_optimizations_config() -> [Any]:
    """
    从 config/optimization.yaml 中获取环境的全局配置属性。
    """
    loader = load_yaml_config("config/optimization.yaml")
    optimizations_configs = loader.get("optimizations", [])
    return optimizations_configs

