"""
LiveMainWindow：QMainWindow + 三个只读表格面板（信号/轮询状态/实盘对照）。

线程模型（重要，决定了下面几个方法各自跑在哪个线程）：
  - `PollWorker`（QThread）跑 `LiveEngine` 轮询循环，触发到策略的 `on_bar()`，
    进而同步触发 `emit_intent()` —— 这条调用链全程在后台线程上。
  - `ib_insync` 用 `util.useQt()` 跟 Qt 主线程的事件循环协作（`live_runner.py`
    里连接前会调用一次），任何 `ib_gateway`/`ib_insync` 调用都必须在主线程做，
    否则会跟 useQt() 接管的事件循环产生竞争。
  - 所以 `_IntentBridge` 存在的唯一理由：把后台线程产生的 `IntentEvent` 通过
    `pyqtSignal` 安全地扔回主线程，真正的真实链匹配（`_on_intent`）只在主线程跑。
  - `Reconciler` 订阅的 `ib.fillEvent` 本身就是在主线程的事件循环上触发的
    （同一个 useQt() 集成），`_on_reconciliation_record` 不需要再跨线程。
"""
import logging
from datetime import datetime, timedelta

from PyQt6.QtCore import QObject, pyqtSignal
from PyQt6.QtWidgets import (QHeaderView, QLabel, QMainWindow, QTableView,
                             QTabWidget, QVBoxLayout, QWidget)

from qib_trader.gui.models import PollStatusTableModel, ReconciliationTableModel, SignalTableModel
from qib_trader.gui.poll_worker import PollWorker
from qib_trader.live.engine import LiveEngine
from qib_trader.live.ib_gateway import IBGateway
from qib_trader.live.intent import IntentEvent, clear_intent_sink, set_intent_sink
from qib_trader.live.option_match import match_real_contract
from qib_trader.live.position_book import PositionBook, RealContract
from qib_trader.live.reconciliation import MatchedIntent, Reconciler
from qib_trader.utils.config_loader import get_codeId_config, get_iv

logger = logging.getLogger("LiveGUI")

# 跟策略层 risk_free_rate 参数的共享默认值保持一致（所有策略类都用 0.04 做默认）；
# IntentEvent 本身不携带这个字段，真实链匹配这里没有更好的来源，用同一个默认值。
_DEFAULT_RISK_FREE_RATE = 0.04


class _IntentBridge(QObject):
    """LiveIntentSink 的 GUI 实现：只做"把 IntentEvent 从后台线程扔到主线程"
    这一件事，不在这里做任何 ib_gateway 调用（那些必须在主线程做，见模块头说明）。"""

    intent_received = pyqtSignal(object)

    def on_intent(self, intent: IntentEvent) -> None:
        self.intent_received.emit(intent)


class LiveMainWindow(QMainWindow):

    def __init__(self, ib_config: dict, strategies: list, poll_buffer_seconds: int,
                reconciliation_window_hours: float, parent=None):
        super().__init__(parent)
        self.setWindowTitle("实盘信号系统 Phase 1（只打印不下单）")
        self.resize(1400, 800)

        self._gateway = IBGateway(ib_config['host'], ib_config['port'],
                                  ib_config.get('client_id', 17))
        self._reconciliation_window = timedelta(hours=reconciliation_window_hours)
        self._position_book = PositionBook()
        self._reconciler: Reconciler = None   # IB 连接成功后才创建

        self._signal_model = SignalTableModel(self)
        self._poll_model = PollStatusTableModel(self)
        self._recon_model = ReconciliationTableModel(self)
        self._build_ui()

        self._engine = LiveEngine(strategies, poll_buffer_seconds=poll_buffer_seconds)

        self._bridge = _IntentBridge()
        self._bridge.intent_received.connect(self._on_intent)

        self._worker = PollWorker(self._engine)
        self._worker.poll_result.connect(self._on_poll_result)
        self._worker.error.connect(self._on_worker_error)

    # ── 启动/关闭 ─────────────────────────────────────────────────────────────

    def start(self) -> None:
        self._connect_ib()
        set_intent_sink(self._bridge)
        self._worker.start()
        self._refresh_status_bar()

    def closeEvent(self, event) -> None:
        clear_intent_sink()
        self._worker.stop()
        self._worker.wait(3000)
        self._gateway.disconnect()
        super().closeEvent(event)

    # ── UI 搭建 ──────────────────────────────────────────────────────────────

    def _build_ui(self) -> None:
        tabs = QTabWidget()
        tabs.addTab(self._make_table_tab(self._signal_model), "信号")
        tabs.addTab(self._make_table_tab(self._poll_model), "轮询状态")
        tabs.addTab(self._make_table_tab(self._recon_model), "实盘对照")
        self.setCentralWidget(tabs)
        self.statusBar().showMessage("未连接")

    @staticmethod
    def _make_table_tab(model) -> QWidget:
        container = QWidget()
        layout = QVBoxLayout(container)
        hint = QLabel("以上价格基于 bar 收盘价计算，实际下单前请在 TWS 核对最新报价")
        layout.addWidget(hint)
        table = QTableView()
        table.setModel(model)
        table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Interactive)
        table.setEditTriggers(QTableView.EditTrigger.NoEditTriggers)
        layout.addWidget(table)
        return container

    def _connect_ib(self) -> None:
        try:
            self._gateway.connect()
            self._reconciler = Reconciler(self._gateway, window=self._reconciliation_window,
                                          on_record=self._on_reconciliation_record)
            logger.info("IB 连接成功，已订阅 fillEvent")
        except Exception as exc:
            logger.error(f"IB 连接失败: {exc}", exc_info=True)

    def _refresh_status_bar(self) -> None:
        if self._gateway.connected:
            accounts = ",".join(self._gateway.managed_accounts())
            self.statusBar().showMessage(f"IB已连接（实盘账户 {accounts}） | Ice数据源正常")
        else:
            self.statusBar().showMessage("IB未连接（真实链匹配/参考报价将不可用） | Ice数据源正常")

    # ── 轮询状态面板 ─────────────────────────────────────────────────────────

    def _on_poll_result(self, result) -> None:
        status = "有新bar" if result.new_bar_count > 0 else "等待中"
        key = (result.code_id, result.interval)
        self._poll_model.upsert_row('key', key, {
            'key': key, 'code_id': result.code_id, 'interval': result.interval,
            'last_poll': result.poll_time.strftime('%Y-%m-%d %H:%M:%S'),
            'last_new_bar': (result.latest_bar_dt.strftime('%Y-%m-%d %H:%M:%S')
                            if result.latest_bar_dt else '-'),
            'status': status,
        })

    def _on_worker_error(self, message: str) -> None:
        logger.error(f"轮询线程错误: {message}")
        self.statusBar().showMessage(f"轮询错误: {message}")

    # ── 信号面板：真实期权链匹配（主线程执行，可以安全调用 ib_gateway） ───────────

    def _on_intent(self, intent: IntentEvent) -> None:
        for leg in intent.legs:
            if leg.action == 'OPEN':
                self._handle_open_leg(intent, leg)
            else:
                self._handle_close_leg(intent, leg)

    def _handle_open_leg(self, intent: IntentEvent, leg) -> None:
        row = {
            'ts': intent.bar_dt.strftime('%Y-%m-%d %H:%M:%S'), 'code_id': intent.code_id,
            'event': intent.event, 'leg': f"OPEN {leg.right} {leg.leg_name or ''}".strip(),
            'target_delta': leg.target_delta, 'target_days': leg.target_days,
            'S': round(intent.underlying_price, 4), 'theo_price': leg.theo_premium,
        }
        match = self._match_leg(intent, leg)
        if match is None:
            row['note'] = '真实链匹配失败（IB未连接或链数据不足），仅展示理论值'
        else:
            row['real_expiry'] = f"{match.expiry}({match.dte_gap:+d})"
            row['real_strike'] = f"{match.strike}({match.strike_gap:+.2f})"
            row['theo_price'] = match.theo_price
            row['ref_quote'] = self._format_ref_quote(intent.code_id, match.expiry, match.strike, leg.right)
            self._position_book.record_open(intent.position_id, leg.leg_name, RealContract(
                code_id=intent.code_id, right=leg.right, expiry=match.expiry,
                strike=match.strike, theo_price=match.theo_price))
            self._register_reconciliation(intent, leg, 'OPEN', match.expiry, match.strike, match.theo_price)
        self._signal_model.prepend_row(row)

    def _match_leg(self, intent: IntentEvent, leg):
        if not self._gateway.connected:
            return None
        try:
            chain = self._gateway.get_option_chain(intent.code_id)
            strike_round = get_codeId_config(intent.code_id).get('strike_round', 1.0)
            # target_delta 为 None 时（比如 dip 机制复用已知的 K2），按经验值取IV，
            # 只做到期日/行权价的双重吸附，不重新按 delta 反推。
            iv_delta = leg.target_delta if leg.target_delta is not None else 0.1
            iv = get_iv(intent.code_id, leg.right, iv_delta)
            return match_real_contract(
                chain, leg.right, intent.underlying_price, _DEFAULT_RISK_FREE_RATE, iv,
                intent.code_id, strike_round, intent.bar_dt,
                target_delta=leg.target_delta, target_days=leg.target_days,
                given_strike=leg.theo_strike if leg.target_delta is None else None)
        except Exception as exc:
            logger.warning(f"真实链匹配失败 {intent.code_id} {leg.right}: {exc}")
            return None

    def _handle_close_leg(self, intent: IntentEvent, leg) -> None:
        row = {
            'ts': intent.bar_dt.strftime('%Y-%m-%d %H:%M:%S'), 'code_id': intent.code_id,
            'event': intent.event, 'leg': f"CLOSE {leg.right} {leg.leg_name or ''}".strip(),
            'S': round(intent.underlying_price, 4), 'theo_price': leg.close_price,
        }
        contract = self._position_book.lookup(intent.position_id, leg.leg_name)
        if contract is None:
            row['note'] = '未找到对应的已开仓真实合约记录（可能是开仓时IB未连接）'
        else:
            row['real_expiry'] = contract.expiry
            row['real_strike'] = contract.strike
            row['ref_quote'] = self._format_ref_quote(intent.code_id, contract.expiry, contract.strike, leg.right)
            self._register_reconciliation(intent, leg, 'CLOSE', contract.expiry, contract.strike, leg.close_price)
            self._position_book.record_close(intent.position_id, leg.leg_name)
        self._signal_model.prepend_row(row)

    def _register_reconciliation(self, intent: IntentEvent, leg, action: str,
                                 expiry: str, strike: float, theo_price: float) -> None:
        if self._reconciler is None:
            return
        self._reconciler.register_intent(MatchedIntent(
            position_id=intent.position_id, leg_name=leg.leg_name, strategy_id=intent.strategy_id,
            event=intent.event, action=action, code_id=intent.code_id, right=leg.right,
            expiry=expiry, strike=strike, theo_price=theo_price,
            intent_time=datetime.now()))   # 用真实处理时刻，不用bar_dt——后者是历史K线时间戳，
                                           # 会比这条建议真正展示给用户的时间早最多一个轮询周期

    def _format_ref_quote(self, code_id: str, expiry: str, strike: float, right: str) -> str:
        if not self._gateway.connected:
            return ''
        quote = self._gateway.get_reference_quote(code_id, expiry, strike, right)
        if quote is None:
            return ''
        return f"{quote['bid']}/{quote['ask']} (mid {quote['mid']})"

    # ── 实盘对照面板 ─────────────────────────────────────────────────────────

    def _on_reconciliation_record(self, record) -> None:
        matched = record.matched
        self._recon_model.prepend_row({
            'fill_time': record.fill_time.strftime('%Y-%m-%d %H:%M:%S'),
            'code_id': matched.code_id, 'event': matched.event,
            'leg': f"{matched.action} {matched.right} {matched.leg_name or ''}".strip(),
            'theo_price': matched.theo_price, 'fill_price': record.fill_price,
            'price_diff': record.price_diff, 'price_diff_pct': record.price_diff_pct,
            'delay_seconds': round(record.delay_seconds, 0), 'fill_side': record.fill_side,
        })
