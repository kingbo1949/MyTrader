import logging
from qib_trader.core.models import BarDatas
from qib_trader.core.broker import get_broker
from qib_trader.core.strategy_pool import get_pool

logger = logging.getLogger(__name__)


class BacktestEngine:

    def run_backtest(self, bars: BarDatas) -> None:
        if not bars:
            logger.warning("没有可用的 Bar 数据，回测停止。")
            return

        pool = get_pool()
        broker = get_broker()

        logger.info(f"开始回测，数据总量: {len(bars)} 条")

        pool.on_init()
        pool.on_start()

        for bar in bars:
            broker.on_bar(bar)   # 先撮合上一根 Bar 挂出的委托
            pool.on_bar(bar)     # 再驱动策略产生新委托

        pool.on_stop()

        logger.info("回测结束。")
