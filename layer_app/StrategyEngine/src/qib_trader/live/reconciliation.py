"""
思路A：真实成交对照——本阶段验证"回测策略映射到实盘之后，理论 vs 真实到底差多少"
的核心模块。

`ib.fillEvent` 是只读推送事件：不管这笔单是不是通过 API 下的，只要是同一账户在
TWS 里手工成交的，都能收到（合约/价格/数量/时间）。`Reconciler` 把这些真实成交
跟系统之前打印过的"建议"（`live/intent.py` 产生、经过真实链匹配之后的意图）配对，
生成"理论 vs 真实"的偏差记录。

注意：本文件不 import ib_insync——回调里的 trade/fill 按鸭子类型使用
（`trade.contract.symbol/lastTradeDateOrContractMonth/strike/right`，
`fill.execution.price/time/side`），保持跟连接细节解耦，也避免任何下单相关
类型出现在这一层。

已知限制：`ib.fillEvent` 只在连接期间实时推送，程序重启期间发生的成交会漏掉；
`ib.reqExecutions()` 可以在启动时补查一次作为"补漏"，属于后续增强，本次不实现。
"""
import logging
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone

logger = logging.getLogger("Reconciliation")

_DEFAULT_WINDOW = timedelta(hours=48)


@dataclass
class MatchedIntent:
    """一条已经完成真实链匹配、正等待与真实成交配对的意图腿。
    由 GUI 侧 sink 在处理完 IntentEvent 的每条 LegIntent 之后调用 register_intent() 写入。
    """
    position_id: object
    leg_name: str
    strategy_id: str
    event: str              # 原 IntentEvent.event，仅供展示
    action: str              # 'OPEN' | 'CLOSE'
    code_id: str
    right: str                # 'call' | 'put'
    expiry: str                # YYYYMMDD
    strike: float
    theo_price: float          # OPEN 用 theo_premium，CLOSE 用 close_price
    intent_time: datetime


@dataclass
class ReconciliationRecord:
    matched: MatchedIntent
    fill_price: float
    fill_qty: float
    fill_side: str           # ib_insync Execution.side: 'BOT'/'SLD'
    fill_time: datetime
    price_diff: float = field(init=False)       # 真实 - 理论
    price_diff_pct: float = field(init=False)     # 相对理论价的百分比
    delay_seconds: float = field(init=False)       # 真实成交时间 - 意图产生时间

    def __post_init__(self):
        self.price_diff = round(self.fill_price - self.matched.theo_price, 4)
        self.price_diff_pct = (round(self.price_diff / self.matched.theo_price * 100, 2)
                               if self.matched.theo_price else None)
        self.delay_seconds = (self.fill_time - self.matched.intent_time).total_seconds()


@dataclass
class ReconciliationStats:
    sample_count: int
    avg_price_diff: float
    avg_price_diff_pct: float
    avg_delay_seconds: float
    unmatched_count: int


class Reconciler:

    def __init__(self, gateway, window: timedelta = _DEFAULT_WINDOW, on_record=None):
        """gateway: IBGateway 实例（用 subscribe_fills 订阅只读的 fillEvent）。
        on_record: 可选回调 (ReconciliationRecord) -> None，供 GUI 主线程更新表格用。"""
        self._window = window
        self._on_record = on_record
        self._pending: list[MatchedIntent] = []
        self._records: list[ReconciliationRecord] = []
        self._unmatched: list = []
        gateway.subscribe_fills(self._on_fill)

    def register_intent(self, matched: MatchedIntent) -> None:
        self._pending.append(matched)

    def _on_fill(self, trade, fill) -> None:
        contract = trade.contract
        execution = fill.execution
        right = 'call' if contract.right and contract.right.upper().startswith('C') else 'put'
        now = execution.time if execution.time.tzinfo else execution.time.replace(tzinfo=timezone.utc)

        candidates = [
            m for m in self._pending
            if m.code_id == contract.symbol
            and m.expiry == contract.lastTradeDateOrContractMonth
            and abs(m.strike - contract.strike) < 1e-6
            and m.right == right
            and (now - m.intent_time) <= self._window
        ]
        if not candidates:
            logger.info(f"未匹配成交: {contract.symbol} {contract.lastTradeDateOrContractMonth} "
                       f"{contract.strike}{contract.right} @ {execution.price}")
            self._unmatched.append((trade, fill))
            return

        matched = max(candidates, key=lambda m: m.intent_time)
        self._pending.remove(matched)
        record = ReconciliationRecord(matched=matched, fill_price=execution.price,
                                      fill_qty=execution.shares, fill_side=execution.side,
                                      fill_time=now)
        self._records.append(record)
        logger.info(f"对照配对成功: {matched.event}/{matched.leg_name} 理论={matched.theo_price} "
                   f"真实={record.fill_price} 差值={record.price_diff}({record.price_diff_pct}%) "
                   f"延迟={record.delay_seconds:.0f}s")
        if self._on_record is not None:
            self._on_record(record)

    def records(self) -> list:
        return list(self._records)

    def unmatched(self) -> list:
        return list(self._unmatched)

    def stats(self) -> ReconciliationStats:
        if not self._records:
            return ReconciliationStats(sample_count=0, avg_price_diff=0.0, avg_price_diff_pct=0.0,
                                       avg_delay_seconds=0.0, unmatched_count=len(self._unmatched))
        n = len(self._records)
        diffs = [r.price_diff for r in self._records]
        pcts = [r.price_diff_pct for r in self._records if r.price_diff_pct is not None]
        delays = [r.delay_seconds for r in self._records]
        return ReconciliationStats(
            sample_count=n,
            avg_price_diff=round(sum(diffs) / n, 4),
            avg_price_diff_pct=round(sum(pcts) / len(pcts), 2) if pcts else 0.0,
            avg_delay_seconds=round(sum(delays) / n, 1),
            unmatched_count=len(self._unmatched),
        )
