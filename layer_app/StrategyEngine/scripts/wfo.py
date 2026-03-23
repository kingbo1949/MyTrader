"""
Walk-Forward Optimization (WFO)
每个窗口：in-sample 网格搜索最优参数 → out-of-sample 单次回测
最终拼接所有 OOS 成交，输出综合绩效报告。
"""
import logging
import sys
import os
import itertools
from dataclasses import dataclass
from datetime import datetime
from multiprocessing import Pool, cpu_count

import IBTrader
import pandas as pd

from qib_trader.api.converters import IceConverter
from qib_trader.core.broker import make_broker, get_broker, reset_broker
from qib_trader.core.engine import BacktestEngine
from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.models import BarDatas
from qib_trader.core.strategy_pool import make_pool, get_pool, reset_pool
from qib_trader.utils.config_loader import get_wfo_config, get_codeId_config
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging
from qib_trader.utils.stats import (
    PerformanceAnalyzer,
    _build_bar_df, _build_trade_df, _merge_pos_into_bars,
    _calc_equity, _calc_summary, _round_trip_pnls, _calc_trade_metrics,
)

_INITIAL_CAPITAL = 100_000.0
logger = logging.getLogger("WFO")


# ─── 数据结构 ─────────────────────────────────────────────────────────────────

@dataclass(slots=True, frozen=True)
class OptResult:
    params: dict
    recovery_factor: float
    sharpe_ratio: float
    total_return: float
    win_rate: float
    trade_count: int


@dataclass(slots=True, frozen=True)
class WindowPerf:
    idx: int
    oos_s: datetime
    oos_e: datetime
    is_result: OptResult
    oos_result: OptResult


# ─── 子进程工作函数（必须在顶层以支持 multiprocessing pickle）─────────────────

def _run_single(task: tuple) -> OptResult:
    class_name, params, bars, code_id, interval_str, direction_str, multiplier = task
    make_pool()
    make_broker()
    conf = {"class": class_name, "code_id": code_id,
            "interval": interval_str, "direction": direction_str, "params": params}
    get_pool().add_strategy(FactoryStrategy.create_strategy(conf))
    BacktestEngine().run_backtest(bars)
    trades = get_broker().get_trades()
    return _score(params, trades, bars, multiplier)


def _score(params, trades, bars, multiplier) -> OptResult:
    if not trades or not bars:
        return OptResult(params, 0.0, 0.0, 0.0, 0.0, 0)
    trade_df = _build_trade_df(trades)
    bar_df = _calc_equity(
        _merge_pos_into_bars(_build_bar_df(bars), trade_df),
        trade_df, _INITIAL_CAPITAL, multiplier
    )
    s = _calc_summary(bar_df, _INITIAL_CAPITAL)
    tm = _calc_trade_metrics(_round_trip_pnls(trade_df, multiplier))
    return OptResult(params=params,
                     recovery_factor=s["recovery_factor"],
                     sharpe_ratio=s["sharpe"],
                     total_return=s["total_return"],
                     win_rate=tm["win_rate"],
                     trade_count=tm["total_trades"])


# ─── 参数网格 ─────────────────────────────────────────────────────────────────

def _param_grid(grid_cfg: dict) -> list[dict]:
    keys, values = zip(*grid_cfg.items())
    return [dict(zip(keys, combo)) for combo in itertools.product(*values)]


# ─── 时间窗口 ─────────────────────────────────────────────────────────────────

def _add_months(dt: datetime, months: int) -> datetime:
    return (pd.Timestamp(dt) + pd.DateOffset(months=months)).to_pydatetime()


def _generate_windows(start: datetime, end: datetime,
                      is_months: int, oos_months: int) -> list[tuple]:
    windows, is_start = [], start
    while True:
        is_end = _add_months(is_start, is_months)
        oos_end = _add_months(is_end, oos_months)
        if oos_end > end:
            break
        windows.append((is_start, is_end, is_end, oos_end))
        is_start = _add_months(is_start, oos_months)
    return windows


def _filter_bars(bars: BarDatas, start: datetime, end: datetime) -> BarDatas:
    return [b for b in bars if start <= b.datetime < end]


# ─── 数据拉取 ─────────────────────────────────────────────────────────────────

def _fetch_bars(cfg: dict) -> BarDatas:
    time_type = getattr(IBTrader.ITimeType, cfg["interval"])
    time_pair = FactoryIce.make_timepair(cfg["start_time"], cfg["end_time"])
    df = FactoryIce.get_enriched_klines_loop(cfg["code_id"], time_type, time_pair)
    if df.empty:
        return []
    return IceConverter.enriched_df_to_bars(df, cfg["code_id"], cfg["interval"])


# ─── IS 最优参数 ──────────────────────────────────────────────────────────────

def _best_params(bars_is: BarDatas, cfg: dict, multiplier: float) -> tuple[dict, OptResult]:
    grid = _param_grid(cfg["grid"])
    tasks = [(cfg["class"], p, bars_is, cfg["code_id"],
              cfg["interval"], cfg["direction"], multiplier)
             for p in grid]
    with Pool(processes=cpu_count()) as pool:
        results = pool.map(_run_single, tasks)
    metric = cfg.get("optimize_by", "recovery_factor")
    best = max(results, key=lambda r: getattr(r, metric))
    return best.params, best


# ─── OOS 单次回测 ─────────────────────────────────────────────────────────────

def _run_oos(bars_oos: BarDatas, params: dict, cfg: dict) -> list:
    make_pool()
    make_broker()
    reset_pool()      # 清除旧策略
    reset_broker()    # 清除旧成交
    conf = {"class": cfg["class"], "code_id": cfg["code_id"],
            "interval": cfg["interval"], "direction": cfg["direction"],
            "params": params}
    get_pool().add_strategy(FactoryStrategy.create_strategy(conf))
    BacktestEngine().run_backtest(bars_oos)
    return get_broker().get_trades()


# ─── 参数汇总输出 ────────────────────────────────────────────────────────────

def _print_params_summary(records: list, cfg: dict) -> None:
    param_keys = list(cfg["grid"].keys())
    col_w = 14
    header = f"{'窗口':<6} {'OOS区间':<24} {'成交':>6}  " + "  ".join(f"{k:<{col_w}}" for k in param_keys)
    sep = "─" * len(header)
    print(f"\n{'★' * 6}  WFO 各窗口最优参数汇总  {'★' * 6}")
    print(sep)
    print(header)
    print(sep)
    for idx, oos_s, oos_e, n_trades, params in records:
        period = f"{oos_s.date()}~{oos_e.date()}"
        vals = "  ".join(f"{str(params.get(k, '-')):<{col_w}}" for k in param_keys)
        print(f"{idx:<6} {period:<24} {n_trades:>6}  {vals}")
    print(sep + "\n")


def _print_window_performance(perf_records: list[WindowPerf]) -> None:
    print(f"\n{'★' * 6}  WFO 各窗口 IS vs OOS 绩效  {'★' * 6}")
    hdr = (f"{'窗口':<6} {'OOS区间':<24} {'成交':>5}  "
           f"{'IS夏普':>8}  {'IS收益%':>8}  "
           f"{'OOS夏普':>8}  {'OOS收益%':>8}  {'恢复系数':>8}  {'胜率%':>6}")
    sep = "─" * len(hdr)
    print(sep); print(hdr); print(sep)
    for wp in perf_records:
        period = f"{wp.oos_s.date()}~{wp.oos_e.date()}"
        o, i = wp.oos_result, wp.is_result
        print(f"{wp.idx:<6} {period:<24} {o.trade_count:>5}"
              f"  {i.sharpe_ratio:>8.2f}  {i.total_return*100:>7.2f}%"
              f"  {o.sharpe_ratio:>8.2f}  {o.total_return*100:>7.2f}%"
              f"  {o.recovery_factor:>8.2f}  {o.win_rate*100:>5.1f}%")
    print(sep + "\n")


# ─── 单任务 WFO ───────────────────────────────────────────────────────────────

def _run_wfo_task(cfg: dict) -> None:
    multiplier = get_codeId_config(cfg["code_id"]).get("multiplier", 1.0)
    all_bars = _fetch_bars(cfg)
    if not all_bars:
        logger.error("数据为空，跳过")
        return
    start = datetime.strptime(cfg["start_time"], "%Y-%m-%d %H:%M:%S")
    end   = datetime.strptime(cfg["end_time"],   "%Y-%m-%d %H:%M:%S")
    windows = _generate_windows(start, end, cfg["is_months"], cfg["oos_months"])
    logger.info(f"共 {len(windows)} 个滚动窗口，IS={cfg['is_months']}月，OOS={cfg['oos_months']}月")
    all_oos_trades, all_oos_bars, window_records, perf_records = [], [], [], []
    for i, (is_s, is_e, oos_s, oos_e) in enumerate(windows, 1):
        bars_is  = _filter_bars(all_bars, is_s, is_e)
        bars_oos = _filter_bars(all_bars, oos_s, oos_e)
        logger.info(f"窗口{i}: IS {is_s.date()}~{is_e.date()} ({len(bars_is)}bars)"
                    f"  OOS {oos_s.date()}~{oos_e.date()} ({len(bars_oos)}bars)")
        params, is_result = _best_params(bars_is, cfg, multiplier)
        oos_trades = _run_oos(bars_oos, params, cfg)
        oos_result = _score(params, oos_trades, bars_oos, multiplier)
        logger.info(f"窗口{i} OOS 成交: {len(oos_trades)} 笔")
        window_records.append((i, oos_s, oos_e, len(oos_trades), params))
        perf_records.append(WindowPerf(i, oos_s, oos_e, is_result, oos_result))
        all_oos_trades.extend(oos_trades)
        all_oos_bars.extend(bars_oos)
    logger.info(f"WFO完成，OOS总成交: {len(all_oos_trades)} 笔")
    _print_params_summary(window_records, cfg)
    _print_window_performance(perf_records)
    analyzer = PerformanceAnalyzer(all_oos_trades, all_oos_bars,
                                   initial_capital=_INITIAL_CAPITAL,
                                   contract_multiplier=multiplier)
    df = analyzer.calculate_performance()
    if not df.empty:
        dd_idx = df["drawdown"].idxmin()
        peak_idx = df.loc[:dd_idx, "equity"].idxmax()
        print(f"\n最大回撤发生区间:")
        print(f"  峰值: {peak_idx}  净值=${df.loc[peak_idx, 'equity']:,.2f}")
        print(f"  谷底: {dd_idx}  净值=${df.loc[dd_idx,  'equity']:,.2f}")
        print(f"  回撤幅度: {df['drawdown'].min():.2%}\n")
    analyzer.plot_result(df)


# ─── 主入口 ───────────────────────────────────────────────────────────────────

def main() -> None:
    setup_logging()
    try:
        tasks = [t for t in get_wfo_config() if t.get("enable", False)]
        logger.info(f"共发现 {len(tasks)} 个启用 WFO 任务")
        for cfg in tasks:
            logger.info(f">>> 开始 WFO: {cfg['class']}_{cfg['code_id']}")
            _run_wfo_task(cfg)
    except Exception as e:
        logger.error(f"WFO 运行失败: {e}", exc_info=True)
    finally:
        FactoryIce.shutdown()
        logger.info("系统已安全关闭")


if __name__ == "__main__":
    main()
