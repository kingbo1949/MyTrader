import logging
import sys


def setup_logging(level: int = logging.INFO) -> None:
    """
    配置根日志器。

    标准做法：显式构造 Handler + Formatter，挂载到根 logger。
    各业务模块只需 logging.getLogger(__name__)，无需关心配置。

    安全性：检查 handlers 是否已存在，避免重复调用时重复添加。
    """
    root_logger = logging.getLogger()

    if root_logger.handlers:
        return

    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(level)
    handler.setFormatter(logging.Formatter(
        fmt='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    ))

    root_logger.setLevel(level)
    root_logger.addHandler(handler)
