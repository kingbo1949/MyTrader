import logging
import math
import pickle
import IBTrader

from qib_trader.api.converters import IceConverter
from qib_trader.core.engine import BacktestEngine
from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.strategy_pool import make_pool, get_pool
from qib_trader.core.broker import make_broker, get_broker
from qib_trader.core.models import BarData
from qib_trader.utils.factory_ice import FactoryIce
from pathlib import Path
import pandas as pd
from qib_trader.utils.config_loader import get_strategies_config, get_codeId_config, is_export_trades_csv, is_use_cache, is_verbose_trade_log
from qib_trader.utils.log_setup import setup_logging
from qib_trader.utils.stats import PerformanceAnalyzer
from signal_character_plot import plot_closed_positions

logger = logging.getLogger("MainBacktest")


def _build_data_key_map(active_configs: list) -> dict:
    """按 (code_id, interval) 去重，合并时间范围。"""
    data_key_map: dict[tuple, tuple] = {}
    for s in active_configs:
        key = (s["code_id"], s["interval"])
        start, end = s["start_time"], s["end_time"]
        if key not in data_key_map:
            data_key_map[key] = (start, end)
        else:
            prev_start, prev_end = data_key_map[key]
            data_key_map[key] = (min(prev_start, start), max(prev_end, end))
    return data_key_map


def _cache_path(code_id: str, interval_str: str, start_time: str, end_time: str) -> Path:
    d = Path(__file__).resolve().parents[1] / "datadb"
    d.mkdir(exist_ok=True)
    start = str(start_time)[:10].replace("-", "")
    end   = str(end_time)[:10].replace("-", "")
    return d / f"{code_id}_{interval_str}_{start}_{end}.pkl"


def _fetch_bars(code_id: str, interval_str: str, start_time, end_time) -> list[BarData]:
    """拉取单组 (code_id, interval) 的 bar 列表，失败返回空列表。"""
    cache = _cache_path(code_id, interval_str, start_time, end_time)
    if is_use_cache() and cache.exists():
        logger.info(f"从本地缓存加载: {cache}")
        with open(cache, "rb") as f:
            return pickle.load(f)

    time_type = getattr(IBTrader.ITimeType, interval_str)
    time_pair = FactoryIce.make_timepair(start_time, end_time)
    df = FactoryIce.get_enriched_klines_loop(code_id, time_type, time_pair)
    if df.empty:
        logger.warning(f"未获取到数据: {code_id} {interval_str}，跳过")
        return []
    bars = IceConverter.enriched_df_to_bars(df, code_id, interval_str)
    with open(cache, "wb") as f:
        pickle.dump(bars, f)
    logger.info(f"数据已缓存到: {cache}")
    return bars


def _fetch_all_bars(data_key_map: dict) -> dict[tuple, list[BarData]]:
    """批量拉取所有 (code_id, interval) 的数据，返回非空结果。"""
    bars_map: dict[tuple, list[BarData]] = {}
    for (code_id, interval_str), (start_time, end_time) in data_key_map.items():
        logger.info(f"拉取数据: {code_id} {interval_str} [{start_time} ~ {end_time}]")
        bars = _fetch_bars(code_id, interval_str, start_time, end_time)
        if bars:
            bars_map[(code_id, interval_str)] = bars
            logger.info(f"  获取 {len(bars)} 根 bar")
    return bars_map


def _register_strategies(active_configs: list, bars_map: dict) -> None:
    """将所有有数据的策略注册到统一策略池。"""
    pool = get_pool()
    for s_config in active_configs:
        key = (s_config["code_id"], s_config["interval"])
        if key not in bars_map:
            logger.warning(f"策略 {s_config['class']} 无对应数据，跳过")
            continue
        strategy = FactoryStrategy.create_strategy(s_config)
        strategy._own_bars = bars_map[key]
        pool.add_strategy(strategy)
        logger.info(f"注册策略: {strategy.strategy_id}")


def _merge_bars(bars_map: dict) -> list[BarData]:
    """合并所有 bar 并按时间排序，构成统一时间轴。"""
    all_bars: list[BarData] = []
    for bars in bars_map.values():
        all_bars.extend(bars)
    all_bars.sort(key=lambda b: b.datetime)
    return all_bars


def _output_dir(code_id: str, subdir: str) -> Path:
    d = Path(__file__).resolve().parents[1] / "output1" / code_id / subdir
    d.mkdir(parents=True, exist_ok=True)
    return d


def _export_trades_csv(strategy_id: str, code_id: str, trades: list) -> None:
    """将成交记录导出为 output/<code_id>/deallist/<strategy_id>.csv。"""
    from dataclasses import asdict
    df = pd.DataFrame([asdict(t) for t in trades])
    path = _output_dir(code_id, "deallist") / f"{strategy_id}.csv"
    df.to_csv(path, index=False, float_format='%.4f')
    logger.info(f"成交记录已导出: {path}")


def _export_spreads_csv(strategy_id: str, code_id: str, closed_spreads: list) -> None:
    """将期权仓位生命周期记录导出为 output/<code_id>/deallist/<strategy_id>.csv。"""
    df = pd.DataFrame(closed_spreads)
    path = _output_dir(code_id, "deallist") / f"{strategy_id}.csv"
    df.to_csv(path, index=False, float_format='%.4f')
    logger.info(f"期权仓位记录已导出: {path}")


def _export_roll_log_csv(strategy_id: str, code_id: str, roll_log: list) -> None:
    """将滚仓/展期明细导出为 output/<code_id>/deallist/<strategy_id>_rolls.csv，
    通过 position_id 与主记录（{strategy_id}.csv）关联。"""
    if not roll_log:
        return
    df = pd.DataFrame(roll_log)
    path = _output_dir(code_id, "deallist") / f"{strategy_id}_rolls.csv"
    df.to_csv(path, index=False, float_format='%.4f')
    logger.info(f"滚仓明细已导出: {path}")


def _max_concurrent_positions(closed: list) -> int:
    events = []
    for p in closed:
        events.append((p["entry_dt"], 1))
        events.append((p["settlement_dt"], -1))
    events.sort(key=lambda x: x[0])
    peak, cur = 0, 0
    for _, delta in events:
        cur += delta
        if cur > peak:
            peak = cur
    return peak


def _print_deal_report(strategy) -> None:
    closed = getattr(strategy, "_closed_spreads", None) or getattr(strategy, "_closed_positions", None)
    if not closed:
        logger.info("无交易记录，跳过报告")
        return
    pnls   = [p["final_pnl"] for p in closed]
    n      = len(pnls)
    wins   = [p for p in pnls if p > 0]
    losses = [p for p in pnls if p <= 0]
    wr     = len(wins) / n
    avg    = sum(pnls) / n
    z      = (len(wins) - n * 0.5) / (math.sqrt(n) * 0.5) if n > 0 else 0.0
    peak   = _max_concurrent_positions(closed)
    lines = [
        "-" * 50,
        f"  class               : {strategy.__class__.__name__}",
        f"  max_concurrent_pos  : {peak}",
        f"  win_rate            : {wr:.2%}",
        f"  avg_pnl_points      : {avg:.2f}",
        f"  avg_win_points      : {sum(wins)/len(wins):.2f}" if wins else "  avg_win_points      : N/A",
        f"  avg_loss_points     : {sum(losses)/len(losses):.2f}" if losses else "  avg_loss_points     : N/A",
        f"  expectancy          : {avg:.2f}",
        f"  z_score             : {z:.2f}",
        "-" * 50,
    ]
    logger.info("\n" + "\n".join(lines))


def _print_results(bars_map: dict) -> None:
    """按策略调用 PerformanceAnalyzer 输出绩效报告。"""
    all_trades = get_broker().get_trades()
    for strategy in get_pool().get_strategies():
        trades = [t for t in all_trades if t.strategy_id == strategy.strategy_id]
        bars = bars_map.get((strategy.codeId, strategy.interval.value), [])
        multiplier = get_codeId_config(strategy.codeId).get("multiplier", 1.0)
        logger.info(f"策略 {strategy.strategy_id}: 共 {len(trades)} 笔成交")
        closed_pos = (getattr(strategy, '_closed_positions', None)
                      or getattr(strategy, '_closed_spreads', None) or [])
        mtm_series = getattr(strategy, '_mtm_pnl_series', {})
        analyzer = PerformanceAnalyzer(trades, bars, initial_capital=strategy.initial_capital,
                                        contract_multiplier=multiplier, options_mode=strategy.options_mode,
                                        mtm_pnl_series=mtm_series,
                                        closed_positions=closed_pos)
        df = analyzer.calculate_performance()
        chart_path = _output_dir(strategy.codeId, "deallist") / f"{strategy.strategy_id}.png"
        analyzer.plot_result(df, title=strategy.strategy_id, params=strategy.params,
                             save_path=str(chart_path))
        if is_export_trades_csv() and trades:
            closed = (getattr(strategy, '_closed_spreads', None)
                      or getattr(strategy, '_closed_positions', None))
            if closed:
                _export_spreads_csv(strategy.strategy_id, strategy.codeId, closed)
            else:
                _export_trades_csv(strategy.strategy_id, strategy.codeId, trades)
            _export_roll_log_csv(strategy.strategy_id, strategy.codeId,
                                 getattr(strategy, '_roll_log', None))
        _print_deal_report(strategy)
        csv_path = chart_path.with_suffix(".csv")
        window_hours = strategy.params.get("option_days", 5) * 24
        plot_closed_positions(csv_path, bars, strategy.codeId, window_hours=window_hours)


def main():
    setup_logging()
    if not is_verbose_trade_log():
        logging.getLogger("qib_trader").setLevel(logging.WARNING)
        logging.getLogger("Strategy").setLevel(logging.WARNING)
    make_pool()
    make_broker()
    try:
        active_configs = [s for s in get_strategies_config() if s.get("enable", False)]
        logger.info(f"共发现 {len(active_configs)} 个启用任务")
        if not active_configs:
            logger.warning("无启用任务，退出")
            return

        data_key_map = _build_data_key_map(active_configs)
        logger.info(f"去重后需拉取 {len(data_key_map)} 组数据")

        bars_map = _fetch_all_bars(data_key_map)
        if not bars_map:
            logger.error("所有数据均为空，退出")
            return

        _register_strategies(active_configs, bars_map)
        if not get_pool().get_strategies():
            logger.error("无策略注册成功，退出")
            return

        all_bars = _merge_bars(bars_map)
        logger.info(f"统一时间轴共 {len(all_bars)} 根 bar，开始回测...")
        BacktestEngine().run_backtest(all_bars)
        _print_results(bars_map)

    except Exception as e:
        logger.error(f"全局运行错误: {e}", exc_info=True)
    finally:
        FactoryIce.shutdown()
        logger.info("ICE 连接已关闭，程序退出")


if __name__ == "__main__":
    main()
