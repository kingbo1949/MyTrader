import sys
import logging
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "ice"))

from PyQt6.QtWidgets import QApplication
from ib_insync import util as ib_util

from qib_trader.core.broker import make_broker
from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.strategy_pool import get_pool, make_pool
from qib_trader.gui.main_window import LiveMainWindow
from qib_trader.utils.config_loader import (get_env_config, get_live_ib_config,
                                            get_live_poll_buffer_seconds,
                                            get_live_reconciliation_window_hours,
                                            get_live_strategies_config)
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging

logger = logging.getLogger("LiveRunner")


def _build_strategies() -> list:
    """按 config/live.yaml 创建启用的策略并注册进策略池。不调用 on_init()——
    LiveEngine 构造时会自己做等价的最小初始化（见 live/engine.py 顶部说明）。"""
    pool = get_pool()
    strategies = []
    for s_config in get_live_strategies_config():
        if not s_config.get("enable", False):
            continue
        strategy = FactoryStrategy.create_strategy(s_config)
        pool.add_strategy(strategy)
        strategies.append(strategy)
        logger.info(f"注册实盘策略: {strategy.strategy_id}")
    return strategies


def main():
    setup_logging()

    app = QApplication(sys.argv)
    ib_util.useQt('PyQt6')   # ib_insync 的事件循环跟 Qt 主循环协作，任何 ib_gateway
                             # 调用都必须留在主线程，见 gui/main_window.py 头部说明

    # LivePrintBroker：只记账不下单。用独立的 mode 值（不是 "real"），避免跟未来
    # 真正的自动下单阶段混用造成误触发下单的风险，见 core/broker.py 顶部说明。
    # 这里直接改写 get_env_config() 返回的缓存字典——只影响本进程，main_backtest.py
    # 等回测脚本是独立进程，不受影响。
    get_env_config()["mode"] = "live_signal"
    make_pool()
    make_broker()

    strategies = _build_strategies()
    if not strategies:
        logger.error("config/live.yaml 中没有已启用（enable: true）的策略，退出")
        return

    window = LiveMainWindow(
        ib_config=get_live_ib_config(),
        strategies=strategies,
        poll_buffer_seconds=get_live_poll_buffer_seconds(),
        reconciliation_window_hours=get_live_reconciliation_window_hours(),
    )
    window.show()
    window.start()

    try:
        exit_code = app.exec()
    finally:
        FactoryIce.shutdown()
        logger.info("ICE 连接已关闭，程序退出")
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
