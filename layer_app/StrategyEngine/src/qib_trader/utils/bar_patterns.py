from dataclasses import dataclass
from qib_trader.core.models import BarData

_BOTTOM_TYPES = ('Bottom', 'BottomSub')
_TOP_TYPES    = ('Top', 'TopSub')


@dataclass(frozen=True, slots=True)
class MultiUTurnZone:
    zone_type: str           # 'bottom' 或 'top'
    zone_start: int          # bars 列表中第一个同类 uturn 的索引
    zone_end: int            # bars 列表中当前 bar 的索引（= len(bars)-1）
    zone_bars: list          # bars[zone_start : zone_end+1]
    uturn_count: int         # 区间内同类 uturn 总数（含当前 bar）
    anchor_bar: BarData      # 回溯终止的反向 bar
    is_mix: bool             # 底部区间内出现过macd>0；顶部区间内出现过macd<0


def detect_multi_uturn(bars: list[BarData], min_count: int = 2) -> MultiUTurnZone | None:
    """检测多重 uturn 形态（底部或顶部）。

    底部：bars[-1] 是底部 uturn → 向前找最近的 Top/TopSub 作锚 → 锚后底部 uturn ≥ min_count
    顶部：bars[-1] 是顶部 uturn → 向前找最近的 Bottom/BottomSub 作锚 → 锚后顶部 uturn ≥ min_count
    返回 MultiUTurnZone 或 None。
    """
    if not bars:
        return None
    current = bars[-1]
    if current.div_type in _BOTTOM_TYPES and current.is_uturn:
        return _detect_zone(bars, min_count, _BOTTOM_TYPES, _TOP_TYPES, 'bottom')
    if current.div_type in _TOP_TYPES and current.is_uturn:
        return _detect_zone(bars, min_count, _TOP_TYPES, _BOTTOM_TYPES, 'top')
    return None


def _detect_zone(bars, min_count, signal_types, anchor_types, zone_type) -> MultiUTurnZone | None:
    """严格按定义：找最近的反向锚点，统计锚后同类 uturn。无锚点则返回 None。"""
    anchor_idx = None
    for i in range(len(bars) - 2, -1, -1):
        if bars[i].div_type in anchor_types:
            anchor_idx = i
            break
    if anchor_idx is None:
        return None
    first_idx = None
    count = 0
    for i in range(anchor_idx + 1, len(bars)):
        if bars[i].div_type in signal_types and bars[i].is_uturn:
            count += 1
            if first_idx is None:
                first_idx = i
    if count < min_count or first_idx is None:
        return None
    end = len(bars) - 1
    zone_bars = bars[first_idx:end + 1]
    if zone_type == 'bottom':
        is_mix = any(b.macd > 0 for b in zone_bars)
    else:
        is_mix = any(b.macd < 0 for b in zone_bars)
    return MultiUTurnZone(
        zone_type=zone_type,
        zone_start=first_idx,
        zone_end=end,
        zone_bars=zone_bars,
        uturn_count=count,
        anchor_bar=bars[anchor_idx],
        is_mix=is_mix,
    )
