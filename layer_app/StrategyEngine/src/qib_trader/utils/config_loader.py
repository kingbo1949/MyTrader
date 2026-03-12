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


def get_optimizations_config() -> [Any]:
    """
    从 config/optimization.yaml 中获取环境的全局配置属性。
    """
    loader = load_yaml_config("config/optimization.yaml")
    optimizations_configs = loader.get("optimizations", [])
    return optimizations_configs

