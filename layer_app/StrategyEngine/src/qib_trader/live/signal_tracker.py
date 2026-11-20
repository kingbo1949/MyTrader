"""
UTurn 信号时间戳的流式（增量）适配。

回测用的 `_build_sync_timestamps`/`_build_sync_timestamps_top`（见
`strategies/uturn_bsync_credit_put_spread.py`/`uturn_bsync_debit_put_spread.py`）
是"给定 [start,end] 一次性算好一个 set()"的批量实现，实盘没有 end_time，不能直接用。

但拆开看，底层判断逻辑本来就是"只看最近这根 bar 所在的窗口"：
  Level 1：单 bar 字段判断（is_uturn + div_type + dif），不需要历史。
  Level 2/3：`detect_multi_uturn(最近N根bar)` 判断"窗口最后一根 bar 是否构成信号"，
             天然是流式的——每来一根新 bar，把它 append 进滑动窗口，重新判断一次即可。

`LiveSyncTimestampTracker` 就是这套逻辑的流式包装：多品种各自维护窗口，
`is_signal_now()` 做 sync_signals 要求的多品种交集判断。
"""
from collections import deque

from qib_trader.utils.bar_patterns import detect_multi_uturn


class LiveSyncTimestampTracker:
    """
    direction='bottom' 对应 `_build_sync_timestamps`（UTurnBottom，给 `_sync_timestamps`/
    `_dip_close_timestamps` 用）；direction='top' 对应 `_build_sync_timestamps_top`
    （UTurnTop，给 `_dip_reopen_timestamps` 用）。

    使用方式（由 live/engine.py 编排）：
      1. 每来一根相关品种的新 bar，调用 on_bar(bar)。
      2. sync_signals 里的所有品种在同一次轮询里都喂过新 bar 之后，调用 is_signal_now()
         判断"当前这个时间点是否构成同步信号"——语义上对应批量版本里两个品种的
         时间戳集合取交集。
    """

    def __init__(self, signals: list[dict], level: int, window: int, direction: str = 'bottom'):
        if direction not in ('bottom', 'top'):
            raise ValueError(f"direction 必须是 'bottom'/'top'，收到: {direction}")
        self._level = level
        self._window = max(window, 1)
        self._direction = direction
        self._dif_floor = {sig['code_id']: float(sig.get('min_dif_abs', 0.0)) for sig in signals}
        self._history = {code_id: deque(maxlen=self._window) for code_id in self._dif_floor}
        self._hit_now = {code_id: False for code_id in self._dif_floor}

    @property
    def code_ids(self) -> list:
        return list(self._dif_floor.keys())

    @property
    def window(self) -> int:
        """live/engine.py 用它决定启动时要预取多少根历史 bar 来填满滑动窗口
        （否则 Level 2/3 信号要等实盘攒够 window 根 bar 才能第一次生效）。"""
        return self._window

    def on_bar(self, bar) -> None:
        """喂入一根新 bar。非本 tracker 关心的品种（bar.codeId 不在 signals 里）直接忽略。"""
        code_id = bar.codeId
        if code_id not in self._history:
            return
        self._history[code_id].append(bar)
        self._hit_now[code_id] = self._is_hit(code_id, bar)

    def is_signal_now(self) -> bool:
        """所有相关品种是否都在各自最近一根 bar 上命中——对应批量版本的多品种交集。"""
        if not self._hit_now:
            return False
        return all(self._hit_now.values())

    def _is_hit(self, code_id: str, bar) -> bool:
        floor = self._dif_floor[code_id]
        if floor > 0 and abs(bar.dif) < floor:
            return False
        if self._level == 1:
            wanted = ('Bottom', 'BottomSub') if self._direction == 'bottom' else ('Top', 'TopSub')
            return bool(bar.is_uturn) and bar.div_type in wanted
        zone = detect_multi_uturn(list(self._history[code_id]))
        if zone is None or zone.zone_type != self._direction:
            return False
        if self._level == 3 and not zone.is_mix:
            return False
        return True
