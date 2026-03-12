import logging
import IBTrader
from typing import Dict, Any

from qib_trader.api.converters import IceConverter
from qib_trader.core.engine import BacktestEngine
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.config_loader import get_symbol_config, get_strategies_config
from qib_trader.utils.stats import PerformanceAnalyzer
from qib_trader.core.factory_strategy import FactoryStrategy


def setup_logging():
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )


def run_single_strategy_backtest(s_config: Dict[str, Any]):
    logger = logging.getLogger(f"Task.{s_config['name']}")
    time_type = getattr(IBTrader.ITimeType, s_config["interval"])
    logger.info(f"正在提取数据: {s_config['symbol']} {s_config['interval']}")
    klines = FactoryIce.get_klines_loop(
        code_id=s_config["symbol"],
        time_type=time_type,
        time_pair=FactoryIce.make_timepair(s_config["start_time"], s_config["end_time"])
    )
    if not klines:
        logger.warning("未获取到行情数据，跳过此任务")
        return
    df = IceConverter.klines_to_df(klines)
    bars = IceConverter.df_to_bars(df, s_config["symbol"], s_config["interval"])
    engine = BacktestEngine()
    strategy = FactoryStrategy.create_strategy(strategy_conf=s_config, engine=engine)
    engine.add_strategy(strategy)
    logger.info("回测运行中...")
    engine.run_backtest(bars)
    symbol_settings = get_symbol_config(s_config['symbol'])
    multiplier = symbol_settings.get("multiplier", 1.0)
    if engine.total_trades:
        analyzer = PerformanceAnalyzer(
            trades=engine.total_trades,
            bars=bars,
            contract_multiplier=multiplier
        )
        perf_df = analyzer.calculate_performance()
        logger.info(f"回测完成，生成绩效报告")
    else:
        logger.info("无成交记录")


def main():
    setup_logging()
    logger = logging.getLogger("MainControl")
    try:
        strategy_configs = get_strategies_config()
        active_tasks = [s for s in strategy_configs if s.get("enable", False)]
        logger.info(f"共发现 {len(active_tasks)} 个待执行任务")
        for s_config in active_tasks:
            try:
                run_single_strategy_backtest(s_config)
            except Exception as e:
                logger.error(f"任务 {s_config.get('name')} 执行失败: {e}", exc_info=True)
    except Exception as e:
        logger.error(f"全局运行错误: {e}", exc_info=True)
    finally:
        FactoryIce.shutdown()
        logger.info("ICE 连接已关闭，程序退出")


if __name__ == "__main__":
    main()
