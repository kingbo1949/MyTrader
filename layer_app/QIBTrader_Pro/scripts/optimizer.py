import logging
import itertools
import pandas as pd
import numpy as np
from multiprocessing import Pool, cpu_count
from dataclasses import dataclass
from typing import List, Dict, Any, Tuple, Optional

# 导入核心组件
import IBTrader
from qib_trader.api.converters import IceConverter
from qib_trader.core.engine import BacktestEngine
from qib_trader.core.models import BarData, Direction
from qib_trader.utils.stats import PerformanceAnalyzer
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.config_loader import load_yaml_config, get_symbol_config, get_optimizations_config


from qib_trader.core.order_sender import BacktestSender
from qib_trader.strategies import get_strategy_class


@dataclass(slots=True, frozen=True)
class OptimizationResult:
    """存储单次参数运行的核心绩效指标"""
    params: Dict[str, Any]
    total_return: float
    sharpe_ratio: float
    max_drawdown: float
    win_rate: float
    pnl_ratio: float
    recovery_factor: float
    trade_count: int


def run_single_backtest(task: Tuple[str, Dict[str, Any], List[BarData], str, str, str, float]) -> OptimizationResult:
    """
    子进程执行函数：运行单个参数组合的回测。
    [修改] 增加 strategy_class_name 以支持多策略动态优化。
    """
    class_name, params, bars, symbol, interval, direction_limit, multiplier = task

    # 1. 初始化引擎与策略 (利用反射获取类)
    engine = BacktestEngine()

    strategy_class = get_strategy_class(class_name)

    if not strategy_class:
        return OptimizationResult(params, 0, 0, 0, 0, 0, 0, 0)

    strategy_name = f"Opt_{'_'.join(map(str, params.values()))}"

    # [核心重构] 构造函数注入：创建回测专用发单器 [cite: 2026-02-25]
    sender = BacktestSender(engine, strategy_name, symbol)

    # 实例化策略
    strategy = strategy_class(
        name = strategy_name,
        symbol=symbol,
        interval=interval,
        order_sender=sender,  # 注入发单器对象
        direction_limit=direction_limit,
        params=params
    )
    engine.add_strategy(strategy)

    # 2. 执行回测
    engine.run_backtest(bars)

    if not engine.total_trades:
        return OptimizationResult(params, 0, 0, 0, 0, 0, 0, 0)

    # 3. 绩效分析 (传入动态乘数)
    analyzer = PerformanceAnalyzer(
        trades=engine.total_trades,
        bars=bars,
        contract_multiplier=multiplier
    )
    # 此处调用 calculate_performance 会触发数据计算，内部已打印简单报告，优化时建议关闭 print
    perf_df = analyzer.calculate_performance()

    if perf_df.empty:
        return OptimizationResult(params, 0, 0, 0, 0, 0, 0, 0)

    # 4. 提取关键指标
    total_net_pnl = perf_df['equity'].iloc[-1] - analyzer.initial_capital
    total_return = (perf_df['equity'].iloc[-1] / analyzer.initial_capital) - 1

    daily_returns = perf_df['equity'].pct_change().fillna(0)
    sharpe = (daily_returns.mean() / daily_returns.std()) * (252 ** 0.5) if daily_returns.std() != 0 else 0

    max_dd_pct = perf_df['drawdown'].min()
    max_dd_amount = (perf_df['max_equity'] - perf_df['equity']).max()
    recovery_factor = total_net_pnl / max_dd_amount if max_dd_amount != 0 else 0

    # 5. 计算基于平仓的胜率与盈亏比
    pnl_list = []
    temp_pos = 0
    entry_price = 0
    for t in engine.total_trades:
        if temp_pos == 0:
            entry_price = t.price
            temp_pos = 1 if t.direction == Direction.LONG else -1
        else:
            pnl = (t.price - entry_price) * multiplier if temp_pos > 0 else (entry_price - t.price) * multiplier
            pnl_list.append(pnl)
            temp_pos = 0

    pnl_s = pd.Series(pnl_list)
    win_rate = len(pnl_s[pnl_s > 0]) / len(pnl_s) if not pnl_s.empty else 0
    avg_win = pnl_s[pnl_s > 0].mean() if not pnl_s[pnl_s > 0].empty else 0
    avg_loss = abs(pnl_s[pnl_s < 0].mean()) if not pnl_s[pnl_s < 0].empty else 1.0
    pnl_ratio = avg_win / avg_loss

    return OptimizationResult(
        params=params, total_return=total_return, sharpe_ratio=sharpe,
        max_drawdown=max_dd_pct, win_rate=win_rate, pnl_ratio=pnl_ratio,
        recovery_factor=recovery_factor, trade_count=len(engine.total_trades)
    )


def show_chinese_results(results: List[OptimizationResult]):
    """打印中文增强版结果报告"""
    # 按收益风险比 (Recovery Factor) 排序
    sorted_results = sorted(results, key=lambda x: x.recovery_factor, reverse=True)

    print("\n" + "★" * 75)
    print(f"{'参数优化增强报告 (按收益风险比排序)':^70}")
    print("★" * 75)

    header = f"{'排名':<4} {'参数组合':<25} {'夏普':<8} {'胜率':<10} {'盈亏比':<8} {'收益风险比':<8}"
    print(header)
    print("-" * 75)

    for i, res in enumerate(sorted_results[:15]):
        param_str = str(res.params)
        print(f"{i + 1:<6} {param_str:<28} {res.sharpe_ratio:<10.2f} {res.win_rate:<12.2%}"
              f" {res.pnl_ratio:<10.2f} {res.recovery_factor:<8.2f}")
    print("★" * 75 + "\n")


def run_optimization_task(opt_config: Dict[str, Any]):
    """执行单个优化任务单元"""
    logger = logging.getLogger(f"Task.{opt_config['name']}")

    # 1. 准备全局参数
    symbol_cfg = get_symbol_config(opt_config['symbol'])
    multiplier = symbol_cfg.get("multiplier", 1.0)

    # 2. 数据拉取
    logger.info(f"正在获取数据: {opt_config['symbol']} {opt_config['interval']}")
    time_type = getattr(IBTrader.ITimeType, opt_config['interval'])
    klines = FactoryIce.get_klines_loop(
        code_id=opt_config['symbol'],
        time_type=time_type,
        time_pair=FactoryIce.make_timepair(opt_config['start_time'], opt_config['end_time'])
    )
    df = IceConverter.klines_to_df(klines)
    bars = IceConverter.df_to_bars(df, opt_config['symbol'], opt_config['interval'])

    # 3. 生成通用参数网格
    grid_cfg = opt_config['grid']
    keys, values = zip(*grid_cfg.items())
    param_grid = [dict(zip(keys, v)) for v in itertools.product(*values)]

    # 针对双均线的特殊过滤逻辑 (可选)
    if opt_config['class'] == "DoubleMAStrategy":
        param_grid = [p for p in param_grid if p.get('fast_window', 0) < p.get('slow_window', 999)]

    # 4. 包装并行任务
    tasks = [
        (opt_config['class'], p, bars, opt_config['symbol'],
         opt_config['interval'], opt_config['direction_limit'], multiplier)
        for p in param_grid
    ]

    # 5. 执行并行计算
    logger.info(f"启动并行池 (核心数: {cpu_count()})，执行组合: {len(tasks)}")
    with Pool(processes=cpu_count()) as pool:
        results = pool.map(run_single_backtest, tasks)

    # 6. 展示结果
    show_chinese_results(results)


def main():
    """主程序入口：基于配置驱动的优化流"""
    logging.basicConfig(level=logging.INFO)
    logger = logging.getLogger("MainOptimizer")

    try:
        # 加载优化任务配置
        tasks = get_optimizations_config()

        for opt_config in tasks:
            if not opt_config.get("enable", False):
                continue

            logger.info(f">>> 开始任务: {opt_config['name']}")
            run_optimization_task(opt_config)

    except Exception as e:
        logger.error(f"优化器运行失败: {e}", exc_info=True)
    finally:
        FactoryIce.shutdown()
        logger.info("系统已安全关闭")


if __name__ == "__main__":
    main()