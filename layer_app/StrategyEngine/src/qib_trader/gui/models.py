"""
三个面板各自的 QAbstractTableModel。只做展示，不含业务逻辑——所有数据都是
main_window.py 从 live/ 包收到的事件转换成的普通 dict 行。
"""
from PyQt6.QtCore import Qt, QAbstractTableModel, QModelIndex


class _BaseTableModel(QAbstractTableModel):
    """公共骨架：内部持有 list[dict]，最新一行插到最前面，超过上限从尾部丢弃
    （GUI 只是给人盯着看的实时面板，不需要无限保留历史——完整记录走日志）。"""

    _MAX_ROWS = 2000

    def __init__(self, columns: list, parent=None):
        super().__init__(parent)
        self._columns = columns   # [(key, header), ...]
        self._rows: list = []

    def rowCount(self, parent=QModelIndex()) -> int:
        return 0 if parent.isValid() else len(self._rows)

    def columnCount(self, parent=QModelIndex()) -> int:
        return 0 if parent.isValid() else len(self._columns)

    def headerData(self, section, orientation, role=Qt.ItemDataRole.DisplayRole):
        if role != Qt.ItemDataRole.DisplayRole or orientation != Qt.Orientation.Horizontal:
            return None
        return self._columns[section][1]

    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if role != Qt.ItemDataRole.DisplayRole or not index.isValid():
            return None
        key = self._columns[index.column()][0]
        value = self._rows[index.row()].get(key)
        return "" if value is None else str(value)

    def prepend_row(self, row: dict) -> None:
        self.beginInsertRows(QModelIndex(), 0, 0)
        self._rows.insert(0, row)
        self.endInsertRows()
        if len(self._rows) > self._MAX_ROWS:
            last = len(self._rows) - 1
            self.beginRemoveRows(QModelIndex(), last, last)
            self._rows.pop()
            self.endRemoveRows()

    def upsert_row(self, key_field: str, key_value, row: dict) -> None:
        """按 key_field 找已有行原地刷新；找不到则插入新行。轮询状态面板用
        （每个 (品种,周期) 只保留最新一行，不是流水账）。"""
        for i, existing in enumerate(self._rows):
            if existing.get(key_field) == key_value:
                self._rows[i] = row
                top_left = self.index(i, 0)
                bottom_right = self.index(i, self.columnCount() - 1)
                self.dataChanged.emit(top_left, bottom_right)
                return
        self.prepend_row(row)


class SignalTableModel(_BaseTableModel):
    """信号面板：每条新意图（经过真实期权链匹配）插到最上面。"""

    def __init__(self, parent=None):
        super().__init__([
            ('ts', '时间戳'), ('code_id', '品种'), ('event', '事件'), ('leg', '方向'),
            ('target_delta', '目标Δ'), ('target_days', '目标DTE'), ('S', '理论S'),
            ('real_expiry', '真实到期日(Δ)'), ('real_strike', '真实行权价(Δ)'),
            ('theo_price', '理论权利金'), ('ref_quote', '参考真实bid/ask/mid'), ('note', '备注'),
        ], parent)


class PollStatusTableModel(_BaseTableModel):
    """轮询状态面板：每个 (品种,周期) 一行，实时刷新，一眼判断数据是否卡住。"""

    def __init__(self, parent=None):
        super().__init__([
            ('code_id', '品种'), ('interval', '周期'), ('last_poll', '最近轮询时间'),
            ('last_new_bar', '最近新bar时间'), ('status', '状态'),
        ], parent)


class ReconciliationTableModel(_BaseTableModel):
    """实盘对照面板：思路A——真实成交(fills) 与历史意图配对后的逐笔偏差记录。"""

    def __init__(self, parent=None):
        super().__init__([
            ('fill_time', '成交时间'), ('code_id', '品种'), ('event', '事件'), ('leg', '腿'),
            ('theo_price', '理论价'), ('fill_price', '真实成交价'), ('price_diff', '差值($)'),
            ('price_diff_pct', '差值(%)'), ('delay_seconds', '延迟(s)'), ('fill_side', '方向'),
        ], parent)
