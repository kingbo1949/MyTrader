import logging
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
from qib_trader.utils.config_loader import get_strategies_config, get_codeId_config, is_export_trades_csv
from qib_trader.utils.log_setup import setup_logging
from qib_trader.utils.stats import PerformanceAnalyzer

logger = logging.getLogger("MainBacktest")


def _build_data_key_map(active_configs: list) -> tuple[dict, dict]:
    """按 (code_id, interval) 去重，合并时间范围，记录 use_enriched 标志。"""
    data_key_map: dict[tuple, tuple] = {}
    enriched_map: dict[tuple, bool] = {}
    for s in active_configs:
        key = (s["code_id"], s["interval"])
        start, end = s["start_time"], s["end_time"]
        use_enriched = s.get("use_enriched", False)
        if key not in data_key_map:
            data_key_map[key] = (start, end)
            enriched_map[key] = use_enriched
        else:
            prev_start, prev_end = data_key_map[key]
            data_key_map[key] = (min(prev_start, start), max(prev_end, end))
            enriched_map[key] = enriched_map[key] or use_enriched
    return data_key_map, enriched_map


def _fetch_bars(code_id: str, interval_str: str, start_time, end_time,
                use_enriched: bool = False) -> list[BarData]:
    """拉取单组 (code_id, interval) 的 bar 列表，失败返回空列表。"""
    time_type = getattr(IBTrader.ITimeType, interval_str)
    time_pair = FactoryIce.make_timepair(start_time, end_time)
    if use_enriched:
        df = FactoryIce.get_enriched_klines_loop(code_id, time_type, time_pair)
        if df.empty:
            logger.warning(f"未获取到 enriched 数据: {code_id} {interval_str}，跳过")
            return []
        return IceConverter.enriched_df_to_bars(df, code_id, interval_str)
    klines = FactoryIce.get_klines_loop(code_id=code_id, time_type=time_type, time_pair=time_pair)
    if not klines:
        logger.warning(f"未获取到数据: {code_id} {interval_str}，跳过")
        return []
    df = IceConverter.klines_to_df(klines)
    return IceConverter.df_to_bars(df, code_id, interval_str)


def _fetch_all_bars(data_key_map: dict, enriched_map: dict) -> dict[tuple, list[BarData]]:
    """批量拉取所有 (code_id, interval) 的数据，返回非空结果。"""
    bars_map: dict[tuple, list[BarData]] = {}
    for (code_id, interval_str), (start_time, end_time) in data_key_map.items():
        use_enriched = enriched_map.get((code_id, interval_str), False)
        tag = "(enriched)" if use_enriched else ""
        logger.info(f"拉取数据{tag}: {code_id} {interval_str} [{start_time} ~ {end_time}]")
        bars = _fetch_bars(code_id, interval_str, start_time, end_time, use_enriched)
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
        pool.add_strategy(strategy)
        logger.info(f"注册策略: {strategy.strategy_id}")


def _merge_bars(bars_map: dict) -> list[BarData]:
    """合并所有 bar 并按时间排序，构成统一时间轴。"""
    all_bars: list[BarData] = []
    for bars in bars_map.values():
        all_bars.extend(bars)
    all_bars.sort(key=lambda b: b.datetime)
    return all_bars


def _export_trades_csv(strategy_id: str, trades: list) -> None:
    """将成交记录导出为 output/<strategy_id>.csv。"""
    output_dir = Path(__file__).resolve().parents[1] / "output"
    output_dir.mkdir(exist_ok=True)
    from dataclasses import asdict
    df = pd.DataFrame([asdict(t) for t in trades])
    path = output_dir / f"{strategy_id}.csv"
    df.to_csv(path, index=False)
    logger.info(f"成交记录已导出: {path}")


def _print_results(bars_map: dict) -> None:
    """按策略调用 PerformanceAnalyzer 输出绩效报告。"""
    all_trades = get_broker().get_trades()
    for strategy in get_pool().get_strategies():
        trades = [t for t in all_trades if t.strategy_id == strategy.strategy_id]
        bars = bars_map.get((strategy.codeId, strategy.interval.value), [])
        multiplier = get_codeId_config(strategy.codeId).get("multiplier", 1.0)
        logger.info(f"策略 {strategy.strategy_id}: 共 {len(trades)} 笔成交")
        analyzer = PerformanceAnalyzer(trades, bars, contract_multiplier=multiplier)
        df = analyzer.calculate_performance()
        analyzer.plot_result(df)
        if is_export_trades_csv() and trades:
            _export_trades_csv(strategy.strategy_id, trades)


def main():
    setup_logging()
    make_pool()
    make_broker()
    try:
        active_configs = [s for s in get_strategies_config() if s.get("enable", False)]
        logger.info(f"共发现 {len(active_configs)} 个启用任务")
        if not active_configs:
            logger.warning("无启用任务，退出")
            return

        data_key_map, enriched_map = _build_data_key_map(active_configs)
        logger.info(f"去重后需拉取 {len(data_key_map)} 组数据")

        bars_map = _fetch_all_bars(data_key_map, enriched_map)
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
