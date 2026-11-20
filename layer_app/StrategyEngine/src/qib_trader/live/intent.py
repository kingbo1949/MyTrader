"""
实盘意图捕获：策略在关键决策点（开/平/滚仓）产生的"理论目标"事件。

策略代码本身完全不知道自己是在回测还是实盘——emit_intent() 在没有注册 sink 时
是纯粹的 no-op（_active_sink 为 None），backtest/optimizer/wfo 等脚本一行都不用改，
行为不受任何影响。只有 live_runner.py 启动时调用 set_intent_sink() 之后，
这些事件才会被真正处理（→ 真实期权链匹配 → 推给 GUI）。
"""
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from datetime import datetime


@dataclass
class LegIntent:
    """单条腿的开仓或平仓意图。"""
    action: str                    # 'OPEN' | 'CLOSE'
    right: str                     # 'call' | 'put'
    leg_name: str = None           # 'K1_short'/'K2_long'/None（单腿事件不需要）

    # OPEN：策略的理论目标，交给真实期权链匹配（option_match.py）用
    target_delta: float = None
    target_days: int = None
    expiry_weekday: int = -1
    theo_strike: float = None      # 策略按信号bar算出的理论行权价（仅供参考对照）
    theo_premium: float = None
    theo_expiry: datetime = None

    # CLOSE：引用的是之前 OPEN 时 position_book 里记录的真实合约，不重新匹配
    old_strike: float = None
    old_premium: float = None
    close_price: float = None      # 理论买回/卖出价（仍是BS理论价，非真实成交）


@dataclass
class IntentEvent:
    strategy_id: str
    code_id: str
    position_id: int
    event: str                     # 'cc_open'/'profit_roll'/'spread_entry'/... 同 roll_history 的 event 命名
    bar_dt: datetime
    underlying_price: float
    legs: list                     # list[LegIntent]，非空
    roll_count: int = None
    close_reason: str = None


class LiveIntentSink(ABC):
    """live_runner.py 注册的实盘 sink 接口，在 GUI 侧实现（做真实链匹配 + 推表格）。"""

    @abstractmethod
    def on_intent(self, intent: IntentEvent) -> None:
        ...


_active_sink: LiveIntentSink = None


def set_intent_sink(sink: LiveIntentSink) -> None:
    """仅供 live_runner.py 调用一次。回测脚本永远不调用这个函数，_active_sink 天然是 None。"""
    global _active_sink
    _active_sink = sink


def clear_intent_sink() -> None:
    global _active_sink
    _active_sink = None


def emit_intent(strategy, event: str, p: dict, bar, *,
                right: str = 'call',
                old_k: float = None, old_prem: float = None, close_price: float = None,
                new_k: float = None, new_prem: float = None, new_expiry=None,
                target_delta: float = None, target_days: int = None, expiry_weekday: int = -1,
                close_reason: str = None, leg_name: str = None) -> None:
    """策略决策点调用的唯一入口。无 sink 时零开销（提前 return）。

    old_k 非空 → 生成一条 CLOSE 腿（平掉 position_book 里记录的真实合约）。
    new_k 非空 → 生成一条 OPEN 腿（target_delta/target_days 交给真实链匹配）。
    两者都非空（比如 profit_roll）→ 同一个 IntentEvent 里包含 CLOSE+OPEN 两条腿。
    """
    if _active_sink is None:
        return
    legs = []
    if old_k is not None:
        legs.append(LegIntent(action='CLOSE', right=right, leg_name=leg_name,
                              old_strike=old_k, old_premium=old_prem, close_price=close_price))
    if new_k is not None:
        legs.append(LegIntent(action='OPEN', right=right, leg_name=leg_name,
                              target_delta=target_delta, target_days=target_days,
                              expiry_weekday=expiry_weekday,
                              theo_strike=new_k, theo_premium=new_prem, theo_expiry=new_expiry))
    if not legs:
        return
    _active_sink.on_intent(IntentEvent(
        strategy_id=strategy.strategy_id, code_id=strategy.codeId,
        position_id=p.get('position_id'), event=event,
        bar_dt=bar.datetime, underlying_price=bar.close_price,
        legs=legs, roll_count=p.get('roll_count'), close_reason=close_reason,
    ))
