"""
SignalMonitorWindow：轻量GUI，顶部一个"均线逼近监控品种"输入框 + "开始"/"停止"
两个按钮，主体是**按周期分开的5个状态面板区**（M1/M5/M15/M30/H1各一个，横向
并排）——每个区里有5块状态面板：UTurn同步 + MA5/MA20/MA60/MA200，面板只显示
"当前这一刻"的状态（背景色+简短字样），不是滚动日志。不碰`ib_insync`/下单
链路，用`LiveEngine`驱动两类"监控用策略"（`uturn_sync_monitor`/
`ma_approach_monitor`，均不下单），每次轮询后直接读它们的`last_bar`/
`is_active_now`当前状态刷新对应面板——完整的事件轨迹（穿越/同步命中的历史
记录）仍然通过这两个策略类里的`self.logger.info()`写进日志文件
（`utils/log_setup.py`配的`FileHandler`），只是不再往GUI里滚动打印。

轮询节奏：用户要求固定每2秒轮询一次，不要`gui/poll_worker.py`里`PollWorker`
那套"按每个(code_id,interval)自己的bar收盘边界+缓冲秒数"调度（M1大概1分钟
才醒一次）。所以这里不用`PollWorker`，改用本文件里的`_FixedIntervalWorker`，
固定sleep(2秒)后调`LiveEngine.poll_all_now()`（`live/engine.py`里的方法，
忽略bar收盘边界、按取整时间无状态地取当前这一根bar）——
`poll_once()`/`due_keys()`/`next_wake_time()`这套原有调度逻辑完全没改，
`PollWorker`/`LiveMainWindow`那条实盘下单链路不受影响。

线程模型：`_FixedIntervalWorker`是后台`QThread`，`logic()`（更新
`monitor.last_bar`/`is_active_now`）全程在这个后台线程上跑；`tick`这个
`pyqtSignal`每轮询一次就发一条，Qt会把它安全地排队到主线程处理——
`_on_tick`在主线程跑，只需要在这里读这些状态字段刷新面板（读的时候对应的
`logic()`调用早已经在后台线程完整跑完，没有并发写入风险）。
"""
import logging
import threading
import time
from datetime import datetime

from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtWidgets import (QGroupBox, QHBoxLayout, QLabel, QLineEdit, QMainWindow,
                             QPushButton, QVBoxLayout, QWidget)

from qib_trader.core.models import Direction, Interval
from qib_trader.live.engine import LiveEngine
from qib_trader.strategies.ma_approach_monitor import ma_approach_monitor
from qib_trader.strategies.uturn_sync_monitor import uturn_sync_monitor

logger = logging.getLogger("SignalMonitorGUI")

_IDLE_STYLE = "background-color: #3a3a3a; color: #3a3a3a; border: 1px solid #666;"
_GREEN_STYLE = "background-color: #2ecc71; color: white; border: 1px solid #666; font-weight: bold;"
_RED_STYLE = "background-color: #e74c3c; color: white; border: 1px solid #666; font-weight: bold;"


class _StatusPanel(QWidget):
    """一块状态面板：上面是固定的小标题（比如"MA20"），下面是颜色+字样随
    状态变化的指示条——标题常显，指示条默认背景色+无字样，命中状态时才
    变色+显示简短文字。"""

    def __init__(self, title: str, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.setSpacing(2)
        caption = QLabel(title)
        caption.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(caption)
        self._indicator = QLabel("")
        self._indicator.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._indicator.setMinimumHeight(36)
        self._indicator.setStyleSheet(_IDLE_STYLE)
        layout.addWidget(self._indicator)

    def set_idle(self) -> None:
        self._indicator.setText("")
        self._indicator.setStyleSheet(_IDLE_STYLE)

    def set_green(self, text: str) -> None:
        self._indicator.setText(text)
        self._indicator.setStyleSheet(_GREEN_STYLE)

    def set_red(self, text: str) -> None:
        self._indicator.setText(text)
        self._indicator.setStyleSheet(_RED_STYLE)


class _FixedIntervalWorker(QThread):
    """每隔`interval_seconds`秒（默认2秒），不管bar收盘边界，直接调一次
    `engine.poll_all_now()`。跟`gui/poll_worker.py`的`PollWorker`（按bar
    收盘节奏调度）是两种不同的轮询策略，只给这个监控应用用。"""

    tick = pyqtSignal(object)   # 携带本次poll_all_now()的起始时间，供_on_tick算总耗时
    error = pyqtSignal(str)

    def __init__(self, engine: LiveEngine, interval_seconds: float = 2.0, parent=None):
        super().__init__(parent)
        self._tag = f"{id(self):#x}"[-6:]   # 诊断用短标签：日志里能分清是不是同一个worker实例
        self._engine = engine
        self._interval_seconds = interval_seconds
        self._running = True
        logger.info(f"_FixedIntervalWorker[{self._tag}] 构造完毕，绑定 engine=[{engine._tag}]")

    def stop(self) -> None:
        logger.info(f"_FixedIntervalWorker[{self._tag}] stop() 被调用 thread={threading.current_thread().name}")
        self._running = False

    def run(self) -> None:
        logger.info(f"_FixedIntervalWorker[{self._tag}] run() 线程启动 "
                   f"thread={threading.current_thread().name} ident={threading.get_ident()}")
        tick_no = 0
        while self._running:
            tick_no += 1
            poll_start = datetime.now()
            logger.info(f"_FixedIntervalWorker[{self._tag}] 第{tick_no}次轮询开始 "
                       f"poll_start={poll_start:%H:%M:%S.%f}")
            try:
                self._engine.poll_all_now(poll_start)
            except Exception as exc:
                logger.error(f"轮询失败: {exc}", exc_info=True)
                self.error.emit(str(exc))
            else:
                self.tick.emit(poll_start)
            self._sleep(self._interval_seconds)
        logger.info(f"_FixedIntervalWorker[{self._tag}] run() 线程退出，共轮询{tick_no}次")

    def _sleep(self, seconds: float) -> None:
        """分段sleep，每0.2秒检查一次_running，让stop()能及时生效。"""
        remaining = seconds
        while self._running and remaining > 0:
            step = min(0.2, remaining)
            time.sleep(step)
            remaining -= step

_INTERVALS = [Interval.M1, Interval.M5, Interval.M15, Interval.M30, Interval.H1]
# 所有周期统一监控全部4条均线（M1不再排除5周期——用户已确认改为统一）
_MA_PERIODS = (5, 20, 60, 200)
_SYNC_SIGNALS = [{'code_id': 'NQ', 'min_dif_abs': 0.0}, {'code_id': 'ES', 'min_dif_abs': 0.0}]


class SignalMonitorWindow(QMainWindow):

    def __init__(self, poll_interval_seconds: float = 2.0, parent=None):
        super().__init__(parent)
        self.setWindowTitle("NQ/ES UTurn同步 + 均线逼近 监控")
        self.resize(1500, 650)

        self._poll_interval_seconds = poll_interval_seconds
        self._worker: _FixedIntervalWorker | None = None
        self._monitors: list = []
        self._bottom_monitors: dict = {}   # interval字符串 -> uturn_sync_monitor('bottom')
        self._top_monitors: dict = {}      # interval字符串 -> uturn_sync_monitor('top')
        self._ma_monitors: dict = {}       # interval字符串 -> ma_approach_monitor
        self._panels: dict = {}            # interval字符串 -> {'uturn':面板, 5:面板, 20:面板, 60:面板, 200:面板}

        self._build_ui()
        logger.info(f"SignalMonitorWindow 构造完毕，poll_interval_seconds={poll_interval_seconds}")

    def _build_ui(self) -> None:
        central = QWidget()
        layout = QVBoxLayout(central)

        top_row = QHBoxLayout()
        top_row.addWidget(QLabel("均线逼近监控品种："))
        self._symbol_input = QLineEdit("NQ")
        top_row.addWidget(self._symbol_input)
        self._start_btn = QPushButton("开始")
        self._start_btn.setAutoDefault(False)   # 防止在输入框里按Enter"顺带"再触发一次按钮点击，
        self._start_btn.setDefault(False)       # 导致_on_start_clicked()被意外调用两次
        self._start_btn.clicked.connect(self._on_start_clicked)
        top_row.addWidget(self._start_btn)
        self._stop_btn = QPushButton("停止")
        self._stop_btn.setAutoDefault(False)
        self._stop_btn.setDefault(False)
        self._stop_btn.setEnabled(False)   # 还没开始监控时，"停止"不可点
        self._stop_btn.clicked.connect(self._on_stop_clicked)
        top_row.addWidget(self._stop_btn)
        layout.addLayout(top_row)

        boxes_row = QHBoxLayout()
        for interval in _INTERVALS:
            box = QGroupBox(interval.value)
            box_layout = QVBoxLayout(box)
            panels = {}
            uturn_panel = _StatusPanel("UTurn")
            box_layout.addWidget(uturn_panel)
            panels['uturn'] = uturn_panel
            for period in _MA_PERIODS:
                p = _StatusPanel(f"MA{period}")
                box_layout.addWidget(p)
                panels[period] = p
            boxes_row.addWidget(box)
            self._panels[interval.value] = panels
        layout.addLayout(boxes_row)

        self.setCentralWidget(central)
        self.statusBar().showMessage("未开始监控")

    def closeEvent(self, event) -> None:
        logger.info("closeEvent 被调用，开始关闭窗口")
        self._stop_worker()
        logger.info("closeEvent 处理完毕")
        super().closeEvent(event)

    # ── 启动/重启监控 ────────────────────────────────────────────────────────

    def _on_start_clicked(self) -> None:
        logger.info(f"_on_start_clicked 被调用 thread={threading.current_thread().name} "
                   f"旧worker={self._worker._tag if self._worker else None}")
        symbol = self._symbol_input.text().strip().upper() or "NQ"
        self._stop_worker()
        self._monitors = self._build_monitors(symbol)
        self._bottom_monitors = {m.interval.value: m for m in self._monitors
                                 if isinstance(m, uturn_sync_monitor) and m.signal_direction == 'bottom'}
        self._top_monitors = {m.interval.value: m for m in self._monitors
                              if isinstance(m, uturn_sync_monitor) and m.signal_direction == 'top'}
        self._ma_monitors = {m.interval.value: m for m in self._monitors if isinstance(m, ma_approach_monitor)}
        self._reset_panels()
        engine = LiveEngine(self._monitors)
        self._worker = _FixedIntervalWorker(engine, interval_seconds=self._poll_interval_seconds)
        self._worker.tick.connect(self._on_tick)
        self._worker.error.connect(self._on_worker_error)
        self._worker.start()
        logger.info(f"_on_start_clicked 完成，新worker=[{self._worker._tag}] engine=[{engine._tag}]")
        self.statusBar().showMessage(f"监控中：均线品种={symbol}")
        self._start_btn.setEnabled(False)
        self._stop_btn.setEnabled(True)

    def _on_stop_clicked(self) -> None:
        self._stop_worker()
        self._reset_panels()
        self.statusBar().showMessage("已停止")
        self._start_btn.setEnabled(True)
        self._stop_btn.setEnabled(False)

    def _stop_worker(self) -> None:
        """先断开旧worker的信号连接，再stop()+wait()——即使wait()超时、旧线程
        这个QThread对象实际还没真正退出（比如正卡在一次很慢的poll_all_now()里），
        它后续emit的tick/error也不会再回调到这个窗口，不会导致_on_tick()被
        意外地多路调用。"""
        if self._worker is not None:
            tag = self._worker._tag
            self._worker.tick.disconnect(self._on_tick)
            self._worker.error.disconnect(self._on_worker_error)
            self._worker.stop()
            finished = self._worker.wait(3000)
            logger.info(f"_stop_worker: worker[{tag}] wait(3000)返回={finished}"
                       f"{'（超时，线程可能仍在跑最后一轮poll_all_now）' if not finished else ''}")
            self._worker = None

    def _reset_panels(self) -> None:
        for panels in self._panels.values():
            for panel in panels.values():
                panel.set_idle()

    @staticmethod
    def _build_monitors(ma_symbol: str) -> list:
        monitors = []
        for interval in _INTERVALS:
            monitors.append(uturn_sync_monitor(
                codeId='NQ', interval=interval, direction=Direction.LONG,
                params=dict(signal_direction='bottom', sync_signals=_SYNC_SIGNALS,
                            uturn_btm_level=1, multi_uturn_window=50)))
            monitors.append(uturn_sync_monitor(
                codeId='NQ', interval=interval, direction=Direction.SHORT,
                params=dict(signal_direction='top', sync_signals=_SYNC_SIGNALS,
                            uturn_btm_level=1, multi_uturn_window=50,
                            dip_reopen_level=1, profit_roll_wait_reopen=True,
                            dip_reopen_sync=True, dip_reopen_window=50)))
        for interval in _INTERVALS:
            monitors.append(ma_approach_monitor(
                codeId=ma_symbol, interval=interval, direction=Direction.LONG,
                params=dict(periods=_MA_PERIODS, threshold_points=30.0)))
        return monitors

    # ── 轮询结果 → 刷新状态面板 ─────────────────────────────────────────────

    def _on_tick(self, poll_start: datetime) -> None:
        on_tick_start = datetime.now()
        self._refresh_panels()
        now = datetime.now()
        logger.info(f"_on_tick 完成: 推送到界面耗时={(now - on_tick_start).total_seconds():.3f}秒，"
                   f"本轮完整周期(轮询开始->界面推送完成)耗时={(now - poll_start).total_seconds():.3f}秒")

    def _refresh_panels(self) -> None:
        for interval in _INTERVALS:
            panels = self._panels[interval.value]
            self._refresh_uturn_panel(panels['uturn'], interval.value)
            self._refresh_ma_panels(panels, interval.value)

    def _refresh_uturn_panel(self, panel: _StatusPanel, interval_str: str) -> None:
        top = self._top_monitors.get(interval_str)
        bottom = self._bottom_monitors.get(interval_str)
        if top is not None and top.is_active_now:
            panel.set_red("top")
        elif bottom is not None and bottom.is_active_now:
            panel.set_green("bottom")
        else:
            panel.set_idle()

    def _refresh_ma_panels(self, panels: dict, interval_str: str) -> None:
        monitor = self._ma_monitors.get(interval_str)
        bar = monitor.last_bar if monitor is not None else None
        for period in _MA_PERIODS:
            panel = panels[period]
            if bar is None:
                panel.set_idle()
                continue
            ma_value = getattr(bar, f"ma{period}", 0.0)
            if ma_value <= 0:
                panel.set_idle()
                continue
            diff = bar.close_price - ma_value
            if -30 < diff < 0:
                panel.set_red(f"{diff:+.2f}")
            elif 0 < diff < 30:
                panel.set_green(f"{diff:+.2f}")
            else:
                panel.set_idle()

    def _on_worker_error(self, message: str) -> None:
        logger.error(f"轮询线程错误: {message}")
        self.statusBar().showMessage(f"轮询错误: {message}")
