"""
影子仓位簿：position_id (+leg_name) → 已推荐的真实合约。

策略内部的 p 字典（call_k/call_prem 等）永远是理论值，跟回测一样——第一阶段
没有真实成交回传，不会反过来影响策略内部状态机。这个仓位簿只是给 GUI/思路A
对照模块用：给某条腿开仓时匹配到的真实合约是哪个，平仓/滚仓时直接查表复用，
不重新匹配一次真实链（同一张仓位的开平仓理应对应同一张合约）。

已知限制：纯内存字典，进程重启会丢失映射，第一版不做持久化（记入后续待办）。
"""
from dataclasses import dataclass


@dataclass
class RealContract:
    code_id: str
    right: str                 # 'call'/'put'
    expiry: str                 # 真实到期日 YYYYMMDD
    strike: float                # 真实行权价
    theo_price: float = None     # 匹配时刻用真实DTE+真实行权价算出的理论权利金，仅供展示对照


class PositionBook:

    def __init__(self):
        self._book: dict = {}   # (position_id, leg_name) -> RealContract

    @staticmethod
    def _key(position_id, leg_name: str = None):
        return (position_id, leg_name)

    def record_open(self, position_id, leg_name: str, contract: RealContract) -> None:
        self._book[self._key(position_id, leg_name)] = contract

    def lookup(self, position_id, leg_name: str = None) -> RealContract | None:
        return self._book.get(self._key(position_id, leg_name))

    def record_close(self, position_id, leg_name: str = None) -> None:
        """平仓后从簿子里移除——CLOSE 类 intent 处理完之后调用。"""
        self._book.pop(self._key(position_id, leg_name), None)

    def open_legs(self) -> list:
        """当前仍视为持仓中的全部 (position_id, leg_name, RealContract)，供 GUI 展示/调试用。"""
        return [(pid, leg, contract) for (pid, leg), contract in self._book.items()]
