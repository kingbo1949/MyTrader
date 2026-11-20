"""
PollWorker：常驻后台 QThread，跑 LiveEngine 的轮询调度循环。

`FactoryIce` 的 Ice 调用是同步阻塞的，直接放主线程会卡界面，所以单独开一个线程
睡到 `engine.next_wake_time()` 再调 `engine.poll_once()`，通过 pyqtSignal 把结果
推回主线程更新表格。分段 sleep（而不是一次性 `time.sleep(整个间隔)`）是为了让
`stop()` 能在至多 1 秒内生效，不用等到下一个 bar 边界才能真正退出。

注意：`ib_insync` 那一侧完全不在这个线程里——它用 `ib_insync.util.useQt()`
跟 Qt 主线程的事件循环协作（见 `scripts/live_runner.py`），任何 `ib_gateway`
调用都必须留在主线程，这个线程只碰 `LiveEngine`（不 import ib_insync）。
"""
import logging
import time
from datetime import datetime

from PyQt6.QtCore import QThread, pyqtSignal

logger = logging.getLogger("PollWorker")


class PollWorker(QThread):

    poll_result = pyqtSignal(object)   # live.engine.PollResult
    error = pyqtSignal(str)

    def __init__(self, engine, parent=None):
        super().__init__(parent)
        self._engine = engine
        self._running = True

    def stop(self) -> None:
        self._running = False

    def run(self) -> None:
        while self._running:
            if not self._sleep_until(self._engine.next_wake_time()):
                break
            if not self._running:
                break
            try:
                results = self._engine.poll_once(datetime.now())
            except Exception as exc:
                logger.error(f"轮询失败: {exc}", exc_info=True)
                self.error.emit(str(exc))
                continue
            for result in results:
                self.poll_result.emit(result)

    def _sleep_until(self, wake_at: datetime) -> bool:
        """分段 sleep 到 wake_at，每秒检查一次 _running。返回 False 表示提前被 stop()。"""
        while self._running:
            remaining = (wake_at - datetime.now()).total_seconds()
            if remaining <= 0:
                return True
            time.sleep(min(1.0, remaining))
        return False
