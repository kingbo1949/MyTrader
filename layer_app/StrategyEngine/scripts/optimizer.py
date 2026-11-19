import logging
import itertools
import math
import os
import pickle
from dataclasses import dataclass
from multiprocessing import Pool, cpu_count
from pathlib import Path

import IBTrader
from qib_trader.api.converters import IceConverter
from qib_trader.core.broker import make_broker, get_broker, reset_broker
from qib_trader.core.engine import BacktestEngine
from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.models import BarDatas
from qib_trader.core.strategy_pool import make_pool, get_pool, reset_pool
from qib_trader.utils.config_loader import get_optimizations_config, get_codeId_config, is_use_cache
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging
from qib_trader.utils.stats import (
    _build_bar_df, _build_trade_df, _merge_pos_into_bars,
    _calc_equity, _calc_summary, _round_trip_pnls, _calc_trade_metrics,
    _lifecycle_trade_metrics,
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
    calmar_ratio: float   # 年化收益% / 最大回撤%，用于网格排名（原 recovery_factor 美元法已废弃排名用途）
    trade_count: int
    max_concurrent: int
    z_score: float
    avg_pnl: float = 0.0
    profit_factor: float = 0.0
    alpha: float = 0.0
    roll_count: int = 0   # 中间滚仓次数（roll_expiry 记录数，非滚仓策略恒为 0）


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

def _cache_path(code_id: str, interval: str, start_time: str, end_time: str) -> Path:
    d = Path(__file__).resolve().parents[1] / "datadb"
    d.mkdir(exist_ok=True)
    start = start_time[:10].replace("-", "")
    end   = end_time[:10].replace("-", "")
    return d / f"{code_id}_{interval}_{start}_{end}.pkl"


def _fetch_bars(opt_config: dict) -> BarDatas:
    cache = _cache_path(opt_config["code_id"], opt_config["interval"],
                        opt_config["start_time"], opt_config["end_time"])
    if is_use_cache() and cache.exists():
        try:
            with open(cache, "rb") as f:
                return pickle.load(f)
        except Exception:
            cache.unlink(missing_ok=True)   # 文件损坏/截断，删除后重新拉取

    time_type = getattr(IBTrader.ITimeType, opt_config["interval"])
    time_pair = FactoryIce.make_timepair(opt_config["start_time"], opt_config["end_time"])
    df = FactoryIce.get_enriched_klines_loop(opt_config["code_id"], time_type, time_pair)
    if df.empty:
        return []
    bars = IceConverter.enriched_df_to_bars(df, opt_config["code_id"], opt_config["interval"])
    # 原子写入：每个进程用独立 tmp 文件（含 PID），rename 到最终路径
    tmp = cache.with_name(f"{cache.stem}_{os.getpid()}.tmp")
    with open(tmp, "wb") as f:
        pickle.dump(bars, f)
    os.replace(tmp, cache)
    return bars


# ─── 绩效提取（静默，不打印）──────────────────────────────────────────────────

_AUX_REASONS = frozenset({
    'roll_expiry', 'put_roll_expiry', 'put_assignment',
    'put_assignment_rejected', 'put_cancelled_call', 'put_cancelled_force',
})


def _calc_max_concurrent(closed_positions: list[dict]) -> int:
    # 排除中间/辅助记录：它们共享正股 entry_dt，不代表新的独立持仓
    positions = [p for p in closed_positions if p.get('close_reason') not in _AUX_REASONS]
    if not positions:
        return 0
    events = [(p["entry_dt"], 1) for p in positions] + \
             [(p["settlement_dt"], -1) for p in positions]
    events.sort(key=lambda x: (x[0], x[1]))  # 同时刻结算(-1)先于开仓(+1)
    max_c, cur = 0, 0
    for _, delta in events:
        cur += delta
        max_c = max(max_c, cur)
    return max_c


def _calc_z_score(win_count: int, trade_count: int) -> float:
    if trade_count <= 0:
        return 0.0
    return (win_count - trade_count * 0.5) / (math.sqrt(trade_count) * 0.5)


def _extract_metrics(params: dict, trades, bars: BarDatas, multiplier: float,
                     initial_capital: float, options_mode: bool,
                     max_concurrent: int = 0,
                     mtm_pnl_series: dict | None = None,
                     closed_positions: list | None = None) -> OptimizationResult:
    if not trades or not bars:
        return OptimizationResult(params, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0.0)
    trade_df = _build_trade_df(trades)
    bar_df = _calc_equity(
        _merge_pos_into_bars(_build_bar_df(bars), trade_df),
        trade_df, initial_capital, multiplier, options_mode, mtm_pnl_series, closed_positions
    )
    summary = _calc_summary(bar_df, initial_capital, closed_positions)
    # 优先用 lifecycle 记录计算胜率/盈亏比，避免多持仓 FIFO 错配
    if closed_positions:
        tm = _lifecycle_trade_metrics(closed_positions, multiplier)
    else:
        tm = _calc_trade_metrics(_round_trip_pnls(trade_df, multiplier))
    roll_count = sum(1 for p in (closed_positions or [])
                     if p.get('close_reason') in ('roll_expiry', 'put_roll_expiry',
                                                   'put_assignment'))
    return OptimizationResult(
        params=params,
        total_return=summary["total_return"],
        sharpe_ratio=summary["sharpe"],
        max_drawdown=summary["max_dd_pct"],
        win_rate=tm["win_rate"],
        pnl_ratio=tm["pnl_ratio"],
        calmar_ratio=summary["calmar_ratio"],
        trade_count=tm["total_trades"],
        max_concurrent=max_concurrent,
        z_score=_calc_z_score(tm["win_count"], tm["total_trades"]),
        avg_pnl=tm["avg_pnl"],
        profit_factor=tm["profit_factor"],
        alpha=summary["alpha"],
        roll_count=roll_count,
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
    strategy._own_bars = bars
    get_pool().add_strategy(strategy)
    BacktestEngine().run_backtest(bars)
    closed     = getattr(strategy, "_closed_positions", None) or []
    max_conc   = _calc_max_concurrent(closed)
    mtm_series = getattr(strategy, '_mtm_pnl_series', {})
    return _extract_metrics(params, get_broker().get_trades(), bars, multiplier,
                            strategy.initial_capital, strategy.options_mode, max_conc,
                            mtm_series, closed)


# ─── 结果展示 ─────────────────────────────────────────────────────────────────

def _format_results(results: list[OptimizationResult], task_name: str,
                    fixed_keys: set = None) -> str:
    ranked = sorted(results, key=lambda r: r.calmar_ratio, reverse=True)
    lines = [
        f"\n{'★' * 150}",
        f"{'参数优化报告: ' + task_name:^146}",
        f"{'★' * 150}",
        f"{'排名':<5} {'参数组合':<32} {'夏普':<8} {'胜率':<10} {'Z分':<8} {'盈亏比':<8} {'利润因子':<10} {'α(%/yr)':<12} {'最大回撤':<10} {'Calmar比率':<12} {'总收益率':<10} {'单笔均盈亏':<12} {'最大持仓':<10} {'平仓次数':<10} {'滚仓次数'}",
        "-" * 170,
    ]
    for i, r in enumerate(ranked[:100]):
        display_params = {k: v for k, v in r.params.items() if k not in (fixed_keys or set())}
        pf = f"{r.profit_factor:.2f}" if r.profit_factor != float('inf') else "∞"
        roll_str = str(r.roll_count) if r.roll_count > 0 else "-"
        lines.append(
            f"{i+1:<7} {str(display_params):<35} {r.sharpe_ratio:<10.2f}"
            f"{r.win_rate:<12.2%} {r.z_score:<10.2f} {r.pnl_ratio:<10.2f} {pf:<12}"
            f"{r.alpha:<14.2%}"
            f"{r.max_drawdown:<12.2%} {r.calmar_ratio:<14.2f}"
            f"{r.total_return:<12.2%} {r.avg_pnl:<14.2f} {r.max_concurrent:<12} {r.trade_count:<12} {roll_str}"
        )
    lines.append(f"{'★' * 150}\n")
    return "\n".join(lines)


def _sync_suffix(fixed_params: dict) -> str:
    signals = fixed_params.get("sync_signals") or []
    if not signals:
        return ""
    return "_" + "-".join(s["code_id"] for s in signals)


def _save_results(text: str, task_name: str, code_id: str, class_name: str,
                  interval: str, fixed_params: dict | None = None) -> None:
    suffix = _sync_suffix(fixed_params or {})
    output_dir = Path(__file__).resolve().parent.parent / "output" / code_id / "optimizer"
    output_dir.mkdir(parents=True, exist_ok=True)
    filename = f"{class_name}_{code_id}_{interval}_opt{suffix}.txt"
    path = output_dir / filename
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    logging.getLogger("Opt").info(f"优化报告已保存: {path}")


def _show_results(results: list[OptimizationResult], task_name: str,
                  code_id: str, class_name: str, interval: str,
                  fixed_keys: set = None, fixed_params: dict = None) -> None:
    text = _format_results(results, task_name, fixed_keys)
    print(text)
    _save_results(text, task_name, code_id, class_name, interval, fixed_params)


# ─── 单任务编排 ───────────────────────────────────────────────────────────────

def _task_id(opt_config: dict) -> str:
    return f"{opt_config['class']}_{opt_config['code_id']}_{opt_config['interval']}_{opt_config['direction']}"


def _prefetch_aux_bars(opt_config: dict, logger: logging.Logger) -> None:
    """在主进程中预先拉取 sync_signals / vix_code_id 所需的辅助 bar 数据并写入缓存。
    确保 worker 进程启动时缓存文件已完整存在，避免多进程并发写入冲突。
    """
    from qib_trader.strategies.uturn_bsync_credit_put_spread import _load_bars
    fixed = opt_config.get("fixed_params") or {}
    interval = opt_config["interval"]
    start, end = opt_config["start_time"], opt_config["end_time"]
    # sync_signals
    for sig in fixed.get("sync_signals") or []:
        cid = sig.get("code_id", "")
        if cid:
            logger.info(f"预拉取 sync_signal 数据: {cid} {interval}")
            _load_bars(cid, interval, start, end)
    # vix_code_id
    vix_cid = fixed.get("vix_code_id", "")
    if vix_cid:
        logger.info(f"预拉取 VIX 数据: {vix_cid} {interval}")
        _load_bars(vix_cid, interval, start, end)


def _run_opt_task(opt_config: dict) -> None:
    task_id = _task_id(opt_config)
    logger = logging.getLogger(f"Opt.{task_id}")
    multiplier = get_codeId_config(opt_config["code_id"]).get("multiplier", 1.0)
    bars = _fetch_bars(opt_config)
    if not bars:
        logger.error(f"数据为空，跳过: {opt_config['name']}")
        return
    # 在启动并行池之前，主进程预拉取所有辅助数据，确保缓存完整
    _prefetch_aux_bars(opt_config, logger)
    fixed_params = opt_config.get("fixed_params") or {}
    param_grid = _filter_param_grid(_generate_param_grid(opt_config["grid"]))
    logger.info(f"有效参数组合: {len(param_grid)} 个，启动并行池 ({cpu_count()} 核)...")
    tasks = [
        (opt_config["class"], {**fixed_params, **p}, bars, opt_config["code_id"],
         opt_config["interval"], opt_config["direction"], multiplier)
        for p in param_grid
    ]
    total = len(tasks)
    results = []
    with Pool(processes=cpu_count()) as pool:
        for i, r in enumerate(pool.imap_unordered(_run_single_backtest, tasks), 1):
            results.append(r)
            if i % 10 == 0 or i == total:
                logger.info(f"进度: {i}/{total} ({i/total:.0%})")
    _show_results(results, task_id, opt_config["code_id"], opt_config["class"],
                  opt_config["interval"], fixed_keys=set(fixed_params.keys()),
                  fixed_params=fixed_params)


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
