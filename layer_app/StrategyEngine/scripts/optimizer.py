import logging
import itertools
import os
from dataclasses import dataclass
from multiprocessing import Pool, cpu_count

import IBTrader
from qib_trader.api.converters import IceConverter
from qib_trader.core.broker import make_broker, get_broker, reset_broker
from qib_trader.core.engine import BacktestEngine
from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.models import BarDatas
from qib_trader.core.strategy_pool import make_pool, get_pool, reset_pool
from qib_trader.utils.config_loader import get_optimizations_config, get_codeId_config
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging
from qib_trader.utils.stats import (
    _build_bar_df, _build_trade_df, _merge_pos_into_bars,
    _calc_equity, _calc_summary, _round_trip_pnls, _calc_trade_metrics,
)

# ─── 结果数据类 ───────────────────────────────────────────────────────────────

@dataclass(slots=True, frozen=True)
class OptimizationResult:
    params: dict
    total_return: float
    sharpe_ratio: float
    max_drawdown: float
    win_rate: float
    pnl_ratio: float
    recovery_factor: float
    trade_count: int
    max_concurrent: int


# ─── 参数网格 ─────────────────────────────────────────────────────────────────

def _generate_param_grid(grid_cfg: dict) -> list[dict]:
    keys, values = zip(*grid_cfg.items())
    return [dict(zip(keys, combo)) for combo in itertools.product(*values)]


def _filter_param_grid(param_grid: list[dict]) -> list[dict]:
    """过滤无效组合：若同时含 fast_window / slow_window，要求 fast < slow。"""
    return [
        p for p in param_grid
        if p.get("fast_window", 0) < p.get("slow_window", float("inf"))
    ]


# ─── 数据拉取 ─────────────────────────────────────────────────────────────────

def _fetch_bars(opt_config: dict) -> BarDatas:
    time_type = getattr(IBTrader.ITimeType, opt_config["interval"])
    time_pair = FactoryIce.make_timepair(opt_config["start_time"], opt_config["end_time"])
    df = FactoryIce.get_enriched_klines_loop(opt_config["code_id"], time_type, time_pair)
    if df.empty:
        return []
    return IceConverter.enriched_df_to_bars(df, opt_config["code_id"], opt_config["interval"])


# ─── 绩效提取（静默，不打印）──────────────────────────────────────────────────

def _calc_max_concurrent(closed_positions: list[dict]) -> int:
    if not closed_positions:
        return 0
    events = [(p["entry_dt"], 1) for p in closed_positions] + \
             [(p["settlement_dt"], -1) for p in closed_positions]
    events.sort(key=lambda x: (x[0], x[1]))  # 同时刻结算(-1)先于开仓(+1)
    max_c, cur = 0, 0
    for _, delta in events:
        cur += delta
        max_c = max(max_c, cur)
    return max_c


def _extract_metrics(params: dict, trades, bars: BarDatas, multiplier: float,
                     initial_capital: float, options_mode: bool,
                     max_concurrent: int = 0) -> OptimizationResult:
    if not trades or not bars:
        return OptimizationResult(params, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0)
    trade_df = _build_trade_df(trades)
    bar_df = _calc_equity(
        _merge_pos_into_bars(_build_bar_df(bars), trade_df),
        trade_df, initial_capital, multiplier, options_mode
    )
    summary = _calc_summary(bar_df, initial_capital)
    tm = _calc_trade_metrics(_round_trip_pnls(trade_df, multiplier))
    return OptimizationResult(
        params=params,
        total_return=summary["total_return"],
        sharpe_ratio=summary["sharpe"],
        max_drawdown=summary["max_dd_pct"],
        win_rate=tm["win_rate"],
        pnl_ratio=tm["pnl_ratio"],
        recovery_factor=summary["recovery_factor"],
        trade_count=tm["total_trades"],
        max_concurrent=max_concurrent,
    )


# ─── 子进程工作函数（顶层可序列化）────────────────────────────────────────────

def _run_single_backtest(task: tuple) -> OptimizationResult:
    class_name, params, bars, code_id, interval_str, direction_str, multiplier = task
    make_pool()
    make_broker()
    reset_pool()
    reset_broker()
    strategy_conf = {
        "class": class_name, "code_id": code_id,
        "interval": interval_str, "direction": direction_str,
        "params": params,
    }
    strategy = FactoryStrategy.create_strategy(strategy_conf)
    get_pool().add_strategy(strategy)
    BacktestEngine().run_backtest(bars)
    closed = getattr(strategy, "_closed_positions", None) or []
    max_conc = _calc_max_concurrent(closed)
    return _extract_metrics(params, get_broker().get_trades(), bars, multiplier,
                            strategy.initial_capital, strategy.options_mode, max_conc)


# ─── 结果展示 ─────────────────────────────────────────────────────────────────

def _format_results(results: list[OptimizationResult], task_name: str,
                    fixed_keys: set = None) -> str:
    ranked = sorted(results, key=lambda r: r.recovery_factor, reverse=True)
    lines = [
        f"\n{'★' * 100}",
        f"{'参数优化报告: ' + task_name:^96}",
        f"{'★' * 100}",
        f"{'排名':<5} {'参数组合':<32} {'夏普':<8} {'胜率':<10} {'盈亏比':<8} {'最大回撤':<10} {'收益风险比':<12} {'总收益率':<10} {'最大持仓'}",
        "-" * 100,
    ]
    for i, r in enumerate(ranked[:50]):
        display_params = {k: v for k, v in r.params.items() if k not in (fixed_keys or set())}
        lines.append(
            f"{i+1:<7} {str(display_params):<35} {r.sharpe_ratio:<10.2f}"
            f"{r.win_rate:<12.2%} {r.pnl_ratio:<10.2f} {r.max_drawdown:<12.2%} {r.recovery_factor:<14.2f}"
            f"{r.total_return:<12.2%} {r.max_concurrent}"
        )
    lines.append(f"{'★' * 100}\n")
    return "\n".join(lines)


def _save_results(text: str, task_name: str) -> None:
    # task_name 格式: ClassName_CodeId_Interval_Direction
    parts = task_name.split("_")
    code_id = parts[1] if len(parts) > 1 else "unknown"
    output_dir = os.path.join(os.path.dirname(__file__), "..", "output", code_id, "optimizer")
    os.makedirs(output_dir, exist_ok=True)
    filename = "_".join(parts[:3]) + "_opt.txt"
    path = os.path.join(output_dir, filename)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    logging.getLogger("Opt").info(f"优化报告已保存: {path}")


def _show_results(results: list[OptimizationResult], task_name: str,
                  fixed_keys: set = None) -> None:
    text = _format_results(results, task_name, fixed_keys)
    print(text)
    _save_results(text, task_name)


# ─── 单任务编排 ───────────────────────────────────────────────────────────────

def _task_id(opt_config: dict) -> str:
    return f"{opt_config['class']}_{opt_config['code_id']}_{opt_config['interval']}_{opt_config['direction']}"


def _run_opt_task(opt_config: dict) -> None:
    task_id = _task_id(opt_config)
    logger = logging.getLogger(f"Opt.{task_id}")
    multiplier = get_codeId_config(opt_config["code_id"]).get("multiplier", 1.0)
    bars = _fetch_bars(opt_config)
    if not bars:
        logger.error(f"数据为空，跳过: {opt_config['name']}")
        return
    fixed_params = opt_config.get("fixed_params") or {}
    param_grid = _filter_param_grid(_generate_param_grid(opt_config["grid"]))
    logger.info(f"有效参数组合: {len(param_grid)} 个，启动并行池 ({cpu_count()} 核)...")
    tasks = [
        (opt_config["class"], {**fixed_params, **p}, bars, opt_config["code_id"],
         opt_config["interval"], opt_config["direction"], multiplier)
        for p in param_grid
    ]
    with Pool(processes=cpu_count()) as pool:
        results = pool.map(_run_single_backtest, tasks)
    _show_results(results, task_id, fixed_keys=set(fixed_params.keys()))


# ─── 主入口 ───────────────────────────────────────────────────────────────────

def main() -> None:
    setup_logging()
    logger = logging.getLogger("MainOptimizer")
    try:
        tasks = [t for t in get_optimizations_config() if t.get("enable", False)]
        logger.info(f"共发现 {len(tasks)} 个启用优化任务")
        for opt_config in tasks:
            logger.info(f">>> 开始任务: {_task_id(opt_config)}")
            _run_opt_task(opt_config)
    except Exception as e:
        logger.error(f"优化器运行失败: {e}", exc_info=True)
    finally:
        FactoryIce.shutdown()
        logger.info("系统已安全关闭")


if __name__ == "__main__":
    main()
