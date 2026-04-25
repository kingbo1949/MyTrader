"""
auto_test.py — 自动化测试脚本
对 TARGETS × INTERVALS 的每个组合依次执行：优化 → 取最佳参数 → 回测 → 出图
运行前确保 optimization.yaml 中有且仅有一个 enable:true 的策略条目作为模板。
"""
import ast
import logging
import re
import sys
from pathlib import Path

import IBTrader

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from qib_trader.api.converters import IceConverter  # noqa: F401（触发 Ice 初始化）
from qib_trader.core.broker import make_broker, get_broker, reset_broker
from qib_trader.core.engine import BacktestEngine
from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.strategy_pool import make_pool, get_pool, reset_pool
from qib_trader.utils.config_loader import get_optimizations_config, get_codeId_config, is_export_trades_csv
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging
from qib_trader.utils.stats import PerformanceAnalyzer

from qib_trader.utils.config_loader import load_yaml_config

from optimizer import _run_opt_task, _fetch_bars, _task_id

# ─── 硬编码配置（按需修改）────────────────────────────────────────────────────

TARGETS   = ["ES"]
# TARGETS   = ["QQQ", "SPY", "AAPL", "AMD", "AMZN", "AVGO", "COIN", "GOOG", "GS", "INTC", "IWM", "JPM", "MCD", "META", "MSFT", "MSTR", "NVDA", "SMH", "SOXX", "TSLA", "TSM", "WMT"]
# TARGETS   = ["QQQ", "SPY", "GS", "JPM", "MS", "SMH", "TSM"]

INTERVALS = ["M15","M5"]

# ─── 模板加载 ──────────────────────────────────────────────────────────────────

def _validate_targets(targets: list[str]) -> None:
    """校验所有品种均已在 settings.yaml 中配置，否则抛出异常终止。"""
    defined = set(load_yaml_config("config/settings.yaml").get("codeIds", {}).keys())
    missing = [c for c in targets if c not in defined]
    if missing:
        raise RuntimeError(f"以下品种未在 settings.yaml 中配置: {missing}，请先添加后再运行")


def _load_template() -> dict:
    """从 optimization.yaml 读取唯一 enable:true 的条目作为模板。"""
    tasks = [t for t in get_optimizations_config() if t.get("enable", False)]
    if not tasks:
        raise RuntimeError("optimization.yaml 中没有 enable:true 的条目")
    if len(tasks) > 1:
        names = [f"{t.get('class','?')}_{t.get('code_id','?')}_{t.get('interval','?')}" for t in tasks]
        logging.getLogger("AutoTest").warning(f"发现 {len(tasks)} 个 enable:true 条目，使用第一个。条目: {names}")
    return tasks[0]


def _build_opt_config(template: dict, code_id: str, interval: str) -> dict:
    """以模板为基础替换 code_id / interval，构造完整 opt_config。"""
    tid = f"{template['class']}_{code_id}_{interval}_{template.get('direction', 'LONG')}"
    return {
        "class":        template["class"],
        "code_id":      code_id,
        "interval":     interval,
        "direction":    template.get("direction", "LONG"),
        "start_time":   template["start_time"],
        "end_time":     template["end_time"],
        "fixed_params": dict(template.get("fixed_params") or {}),
        "grid":         template["grid"],
        "name":         tid,   # _run_opt_task 在数据为空时引用此字段
    }


# ─── 优化阶段 ──────────────────────────────────────────────────────────────────

def _opt_txt_path(opt_config: dict) -> Path:
    """返回 optimizer 保存结果的 txt 文件路径。"""
    tid   = _task_id(opt_config)
    parts = tid.split("_")
    code_id  = parts[1]
    filename = "_".join(parts[:3]) + "_opt.txt"
    return Path(__file__).resolve().parents[1] / "output" / code_id / "optimizer" / filename


def _parse_best_params(opt_config: dict) -> dict | None:
    """解析优化结果 txt 的第一行参数 dict，合并 fixed_params 后返回。"""
    path  = _opt_txt_path(opt_config)
    fixed = opt_config.get("fixed_params") or {}
    if not path.exists():
        logging.getLogger("AutoTest").error(f"找不到优化结果: {path}")
        return None
    with open(path, encoding="utf-8") as f:
        for line in f:
            if re.match(r"^\s*1\s+\{", line):
                m = re.search(r"\{[^{}]+\}", line)
                if m:
                    return {**fixed, **ast.literal_eval(m.group())}
    return None


# ─── 回测阶段 ──────────────────────────────────────────────────────────────────

def _out_dir(code_id: str, subdir: str) -> Path:
    d = Path(__file__).resolve().parents[1] / "output" / code_id / subdir
    d.mkdir(parents=True, exist_ok=True)
    return d


def _export_csv(strategy, trades: list, code_id: str) -> None:
    """导出 CSV，优先使用期权仓位记录，否则用成交流水。"""
    import pandas as pd
    from dataclasses import asdict
    sid    = strategy.strategy_id
    closed = getattr(strategy, "_closed_spreads", None) or getattr(strategy, "_closed_positions", None)
    if closed:
        pd.DataFrame(closed).to_csv(_out_dir(code_id, "deallist") / f"{sid}.csv",
                                    index=False, float_format='%.4f')
    elif trades:
        pd.DataFrame([asdict(t) for t in trades]).to_csv(
            _out_dir(code_id, "deallist") / f"{sid}.csv", index=False, float_format='%.4f')


def _run_verified_backtest(opt_config: dict, best_params: dict) -> None:
    """用最佳参数跑回测，输出绩效图并导出 CSV。"""
    logger = logging.getLogger(f"AutoTest.{opt_config['code_id']}.{opt_config['interval']}")
    bars   = _fetch_bars(opt_config)
    if not bars:
        logger.error("数据为空，跳过回测")
        return
    reset_pool()
    reset_broker()
    strategy_conf = {
        "class": opt_config["class"], "code_id": opt_config["code_id"],
        "interval": opt_config["interval"], "direction": opt_config["direction"],
        "params": best_params,
    }
    strategy = FactoryStrategy.create_strategy(strategy_conf)
    get_pool().add_strategy(strategy)
    BacktestEngine().run_backtest(bars)
    trades     = [t for t in get_broker().get_trades() if t.strategy_id == strategy.strategy_id]
    multiplier = get_codeId_config(opt_config["code_id"]).get("multiplier", 1.0)
    analyzer   = PerformanceAnalyzer(trades, bars, initial_capital=strategy.initial_capital,
                                     contract_multiplier=multiplier, options_mode=strategy.options_mode)
    df         = analyzer.calculate_performance()
    chart_path = _out_dir(opt_config["code_id"], "deallist") / f"{strategy.strategy_id}.png"
    analyzer.plot_result(df, title=strategy.strategy_id, params=strategy.params,
                         save_path=str(chart_path), show=False)
    if is_export_trades_csv() and trades:
        _export_csv(strategy, trades, opt_config["code_id"])


# ─── 单任务编排 ───────────────────────────────────────────────────────────────

def _run_one(template: dict, code_id: str, interval: str) -> None:
    logger     = logging.getLogger("AutoTest")
    opt_config = _build_opt_config(template, code_id, interval)
    logger.info(f">>> 开始优化: {code_id} {interval}")
    _run_opt_task(opt_config)
    best_params = _parse_best_params(opt_config)
    if not best_params:
        logger.error(f"解析最佳参数失败，跳过 {code_id} {interval}")
        return
    logger.info(f"最佳参数: {best_params}")
    _run_verified_backtest(opt_config, best_params)


# ─── 主入口 ───────────────────────────────────────────────────────────────────

def main() -> None:
    setup_logging()
    logger = logging.getLogger("AutoTest")
    make_pool()
    make_broker()
    try:
        _validate_targets(TARGETS)
        template = _load_template()
        logger.info(f"模板: {template['class']} | 品种: {TARGETS} | 周期: {INTERVALS}")
        for code_id in TARGETS:
            for interval in INTERVALS:
                _run_one(template, code_id, interval)
    except Exception as e:
        logger.error(f"运行失败: {e}", exc_info=True)
    finally:
        FactoryIce.shutdown()
        logger.info("ICE 连接已关闭")


if __name__ == "__main__":
    main()
