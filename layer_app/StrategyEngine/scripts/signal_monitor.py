import sys
import logging
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "ice"))

from PyQt6.QtWidgets import QApplication

from qib_trader.gui.signal_monitor_window import SignalMonitorWindow
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging

logger = logging.getLogger("SignalMonitorRunner")


def main():
    log_dir = Path(__file__).resolve().parents[1] / "logs"
    log_dir.mkdir(exist_ok=True)
    log_file = log_dir / f"signal_monitor_{datetime.now():%Y%m%d_%H%M%S}.log"
    setup_logging(log_file=str(log_file))
    logger.info(f"程序启动，完整事件日志写入: {log_file}")

    app = QApplication(sys.argv)

    window = SignalMonitorWindow(poll_interval_seconds=2.0)
    window.show()
    logger.info("窗口已显示")

    try:
        exit_code = app.exec()
    finally:
        logger.info("应用事件循环退出，开始关闭")
        FactoryIce.shutdown()
        logger.info("ICE 连接已关闭，程序退出")
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
