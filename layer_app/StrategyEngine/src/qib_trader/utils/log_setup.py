import logging
import sys


def setup_logging(level: int = logging.INFO, log_file: str = None) -> None:
    """
    配置根日志器。

    标准做法：显式构造 Handler + Formatter，挂载到根 logger。
    各业务模块只需 logging.getLogger(__name__)，无需关心配置。

    安全性：检查 handlers 是否已存在，避免重复调用时重复添加。

    `log_file`：可选，传入路径时额外挂一个 FileHandler，把同样格式的日志整个
    镜像写进这个文件——跟 stdout 完全并行，不影响任何既有调用方（默认不传，
    行为与之前完全一样）。给需要事后翻查完整事件时间线的场景用（比如
    `scripts/signal_monitor.py`：终端可能被清掉/滚动丢失，文件留一份完整记录）。
    """
    root_logger = logging.getLogger()

    if root_logger.handlers:
        return

    formatter = logging.Formatter(
        fmt='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )

    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(level)
    handler.setFormatter(formatter)

    root_logger.setLevel(level)
    root_logger.addHandler(handler)

    if log_file:
        file_handler = logging.FileHandler(log_file, encoding='utf-8')
        file_handler.setLevel(level)
        file_handler.setFormatter(formatter)
        root_logger.addHandler(file_handler)
