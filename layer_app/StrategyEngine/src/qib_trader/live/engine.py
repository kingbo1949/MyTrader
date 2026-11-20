"""
LiveEngine：实盘轮询调度主循环（纯 Python，不 import Qt——见包头说明）。

核心职责：
  1. 汇总所有已注册策略需要的 (code_id, interval) 集合（不只是 strategy.codeId，
     还包括 sync_signals/dip_close_code_id/dip_reopen_code_id/vix_code_id 里涉及
     的全部品种，因为 on_start() 里 `_build_*_timestamps` 都要用到它们）。
  2. 按整点边界 + 缓冲秒数（默认59秒）调度每个 (code_id, interval) 的轮询时机。
  3. 用 `LiveSyncTimestampTracker` 做 UTurn 信号的流式判断（见 signal_tracker.py），
     判断结果直接写回 `strategy._sync_timestamps`/`_dip_close_timestamps`/
     `_dip_reopen_timestamps` 这几个 set——策略代码本身完全不需要知道自己是在
     回测还是实盘（`_entry_signal` 等判断逻辑读的还是这几个 set）。
  4. 不调用策略原本的 `on_start()`——那个方法会用 `start_time`/`end_time` 做一次性
     历史批量查询来构建时间戳集合，语义上是"整个回测区间"，不适用于实盘。改为
     手动做等价的最小初始化（`on_init()` + 清空时间戳集合 + 启动时预取历史 bar
     填满 tracker 的滑动窗口，见 `_prime_trackers`），然后把新 bar 逐根喂给
     `strategy.on_bar()`。

已知限制：进程重启会丢失轮询游标（`_last_seen_ms`）和 tracker 的滑动窗口状态，
每次重启都要重新预取历史 bar 填窗口；不做持久化（第一版不需要，记入后续待办）。
"""
import logging
import threading
from dataclasses import dataclass
from datetime import datetime, timedelta, time as dtime

import IBTrader

from qib_trader.api.converters import IceConverter
from qib_trader.core.models import BarData
from qib_trader.live.signal_tracker import LiveSyncTimestampTracker
from qib_trader.utils.factory_ice import FactoryIce

logger = logging.getLogger("LiveEngine")

_INTERVAL_MINUTES = {'M1': 1, 'M5': 5, 'M15': 15, 'M30': 30, 'H1': 60}


@dataclass
class PollResult:
    code_id: str
    interval: str
    poll_time: datetime
    new_bar_count: int
    latest_bar_dt: datetime = None


def _floor_boundary(interval_str: str, from_dt: datetime) -> datetime:
    """<= from_dt 的当前 bar 起始边界（取整，不是下一个）。
    例：09:49:31 在 M1 上 -> 09:49:00，在 M5 上 -> 09:45:00。"""
    minutes = _INTERVAL_MINUTES.get(interval_str)
    if minutes is None:   # D1 或未知周期：退化为当天0点
        return datetime.combine(from_dt.date(), dtime(0, 0))
    day_start = datetime.combine(from_dt.date(), dtime(0, 0))
    elapsed = (from_dt - day_start).total_seconds() / 60.0
    slot = int(elapsed // minutes) * minutes
    return day_start + timedelta(minutes=slot)


def _next_boundary(interval_str: str, from_dt: datetime) -> datetime:
    """>= from_dt 的下一个对齐 bar 收盘边界（不含 from_dt 本身）。"""
    minutes = _INTERVAL_MINUTES.get(interval_str)
    if minutes is None:   # D1 或未知周期：退化为下一个午夜
        return datetime.combine((from_dt + timedelta(days=1)).date(), dtime(0, 0))
    day_start = datetime.combine(from_dt.date(), dtime(0, 0))
    elapsed = (from_dt - day_start).total_seconds() / 60.0
    next_slot = (int(elapsed // minutes) + 1) * minutes
    return day_start + timedelta(minutes=next_slot)


def _make_time_pair(begin_ms: int, end_ms: int):
    time_pair = IBTrader.ITimePair()
    time_pair.beginPos = begin_ms
    time_pair.endPos = end_ms
    return time_pair


class LiveEngine:

    def __init__(self, strategies: list, poll_buffer_seconds: int = 59, now: datetime = None):
        self._tag = f"{id(self):#x}"[-6:]   # 诊断用短标签：日志里能分清是不是同一个LiveEngine实例
        self._strategies = strategies
        self._buffer = timedelta(seconds=poll_buffer_seconds)
        self._trackers: dict = {}          # strategy_id -> {'sync':tracker,'dip_close':tracker,'dip_reopen':tracker}
        self._vix_targets: dict = {}        # (code_id, interval) -> [strategy, ...]
        self._last_seen_ms: dict = {}        # (code_id, interval) -> ms
        self._next_wake: dict = {}           # (code_id, interval) -> datetime

        now = now or datetime.now()
        logger.info(f"LiveEngine[{self._tag}] 构造开始: now参数={now:%Y-%m-%d %H:%M:%S.%f} "
                   f"真实时间={datetime.now():%Y-%m-%d %H:%M:%S.%f} thread={threading.current_thread().name}")
        code_interval_set = set()
        for strategy in strategies:
            self._init_strategy_state(strategy)
            self._build_trackers(strategy)
            code_interval_set |= self._required_keys(strategy)

        for key in code_interval_set:
            self._last_seen_ms[key] = int(now.timestamp() * 1000)
            self._next_wake[key] = self._wake_after(key[1], now)

        self._prime_trackers(now)
        logger.info(f"LiveEngine[{self._tag}] 初始化完毕：{len(strategies)} 个策略，"
                   f"{len(code_interval_set)} 组 (品种,周期) 需要轮询，"
                   f"耗时={( datetime.now() - now ).total_seconds():.2f}秒")
        for key in sorted(code_interval_set):
            logger.info(f"LiveEngine[{self._tag}] 构造完毕后 _last_seen_ms[{key}]="
                       f"{datetime.fromtimestamp(self._last_seen_ms[key] / 1000):%Y-%m-%d %H:%M:%S}")

    # ── 初始化 ────────────────────────────────────────────────────────────────

    def _init_strategy_state(self, strategy) -> None:
        """等价于 on_start() 的最小初始化，跳过其中基于 start_time/end_time 的
        历史批量查询——那部分职责由本类的 tracker 机制在实盘增量接管。"""
        strategy.on_init()
        strategy.active = True
        for attr in ('_sync_timestamps', '_dip_close_timestamps', '_dip_reopen_timestamps'):
            if hasattr(strategy, attr):
                setattr(strategy, attr, set())
        if hasattr(strategy, '_vix_map'):
            strategy._vix_map = {}
        logger.info(f"策略状态已初始化: {strategy.strategy_id}")

    def _required_keys(self, strategy) -> set:
        """策略需要驱动的 (code_id, interval) 全集：自身品种 ∪ sync_signals ∪
        dip_close/dip_reopen 单品种模式的品种 ∪ vix_code_id（min_vix>0 时）。"""
        interval = strategy.interval.value
        keys = {(strategy.codeId, interval)}
        sync_signals = getattr(strategy, 'sync_signals', None) or []
        for sig in sync_signals:
            keys.add((sig['code_id'], interval))

        dip_close_level = getattr(strategy, 'dip_close_level', 0)
        if dip_close_level > 0 and not getattr(strategy, 'dip_close_sync', True):
            code_id = getattr(strategy, 'dip_close_code_id', '') or strategy.codeId
            keys.add((code_id, interval))

        dip_reopen_level = getattr(strategy, 'dip_reopen_level', 0)
        need_dip_reopen = dip_reopen_level > 0 and (
            dip_close_level > 0 or getattr(strategy, 'profit_roll_wait_reopen', False))
        if need_dip_reopen and not getattr(strategy, 'dip_reopen_sync', True):
            code_id = getattr(strategy, 'dip_reopen_code_id', '') or strategy.codeId
            keys.add((code_id, interval))

        min_vix = getattr(strategy, 'min_vix', 0.0)
        vix_code_id = getattr(strategy, 'vix_code_id', '')
        if min_vix > 0 and vix_code_id:
            key = (vix_code_id, interval)
            keys.add(key)
            self._vix_targets.setdefault(key, []).append(strategy)
        return keys

    def _build_trackers(self, strategy) -> None:
        """跟 `_cc_dip_mixin.py` 的 `_build_btm_timestamps`/`_build_top_timestamps`
        语义完全对应——sync=True 用 sync_signals，sync=False 用单品种。"""
        sync_signals = getattr(strategy, 'sync_signals', None) or []
        trackers = {}
        if sync_signals:
            trackers['sync'] = LiveSyncTimestampTracker(
                sync_signals, getattr(strategy, 'uturn_btm_level', 1),
                getattr(strategy, 'multi_uturn_window', 50), direction='bottom')

        dip_close_level = getattr(strategy, 'dip_close_level', 0)
        if dip_close_level > 0:
            signals = (sync_signals if getattr(strategy, 'dip_close_sync', True)
                      else [{'code_id': getattr(strategy, 'dip_close_code_id', '') or strategy.codeId,
                             'min_dif_abs': 0.0}])
            trackers['dip_close'] = LiveSyncTimestampTracker(
                signals, dip_close_level, getattr(strategy, 'dip_close_window', 50), direction='bottom')

        dip_reopen_level = getattr(strategy, 'dip_reopen_level', 0)
        need_dip_reopen = dip_reopen_level > 0 and (
            dip_close_level > 0 or getattr(strategy, 'profit_roll_wait_reopen', False))
        if need_dip_reopen:
            signals = (sync_signals if getattr(strategy, 'dip_reopen_sync', True)
                      else [{'code_id': getattr(strategy, 'dip_reopen_code_id', '') or strategy.codeId,
                             'min_dif_abs': 0.0}])
            trackers['dip_reopen'] = LiveSyncTimestampTracker(
                signals, dip_reopen_level, getattr(strategy, 'dip_reopen_window', 50), direction='top')

        self._trackers[strategy.strategy_id] = trackers

    def _prime_trackers(self, now: datetime) -> None:
        """启动时用历史 bar 预填 tracker 的滑动窗口，避免 Level 2/3 信号要等实盘
        攒够 window 根 bar 才第一次生效。同时把预取到的最新 bar 时间推进
        `_last_seen_ms`，避免正式轮询时重复拉取这批历史数据。"""
        needed: dict = {}   # (code_id, interval) -> 需要的最大 window
        for strategy in self._strategies:
            interval = strategy.interval.value
            for tracker in self._trackers.get(strategy.strategy_id, {}).values():
                for code_id in tracker.code_ids:
                    key = (code_id, interval)
                    needed[key] = max(needed.get(key, 0), tracker.window)

        for (code_id, interval_str), window in needed.items():
            bars = self._fetch_priming_bars(code_id, interval_str, window, now)
            if not bars:
                logger.warning(f"预填历史数据为空: {code_id} {interval_str}，"
                              f"Level2/3 信号需要实盘攒够 {window} 根 bar 才会生效")
                continue
            for strategy in self._strategies:
                if strategy.interval.value != interval_str:
                    continue
                for tracker in self._trackers.get(strategy.strategy_id, {}).values():
                    if code_id in tracker.code_ids:
                        for bar in bars:
                            tracker.on_bar(bar)
            key = (code_id, interval_str)
            before_ms = self._last_seen_ms.get(key, 0)
            latest_ms = int(bars[-1].datetime.timestamp() * 1000)
            self._last_seen_ms[key] = max(before_ms, latest_ms)
            logger.info(
                f"LiveEngine[{self._tag}] 预填历史数据完成: {code_id} {interval_str} 共 {len(bars)} 根 bar，"
                f"范围[{bars[0].datetime} ~ {bars[-1].datetime}]，"
                f"_last_seen_ms: {datetime.fromtimestamp(before_ms / 1000) if before_ms else None} -> "
                f"{datetime.fromtimestamp(self._last_seen_ms[key] / 1000)}")

    def _fetch_priming_bars(self, code_id: str, interval_str: str, window: int, now: datetime) -> list:
        minutes = _INTERVAL_MINUTES.get(interval_str, 1440)
        lookback_minutes = max(minutes * window * 4, 2 * 1440)
        start_dt = now - timedelta(minutes=lookback_minutes)
        bars = self._fetch_bars_between(code_id, interval_str, start_dt, now)
        return bars[-window:] if len(bars) > window else bars

    # ── 轮询调度 ──────────────────────────────────────────────────────────────

    def _wake_after(self, interval_str: str, from_dt: datetime) -> datetime:
        boundary = _next_boundary(interval_str, from_dt)
        wake = boundary + self._buffer
        while wake <= from_dt:
            boundary = _next_boundary(interval_str, boundary)
            wake = boundary + self._buffer
        return wake

    def next_wake_time(self) -> datetime:
        """所有 (品种,周期) 里最早的下一次轮询时间，调用方（GUI的poll_worker）
        用它决定睡多久。"""
        return min(self._next_wake.values())

    def due_keys(self, now: datetime = None) -> list:
        now = now or datetime.now()
        return [key for key, wake in self._next_wake.items() if wake <= now]

    def _fetch_bars_between(self, code_id: str, interval_str: str,
                           start_dt: datetime, end_dt: datetime) -> list:
        time_type = getattr(IBTrader.ITimeType, interval_str)
        begin_ms = int(start_dt.timestamp() * 1000)
        end_ms = int(end_dt.timestamp() * 1000) + 1   # 半开区间[begin,end)，+1覆盖end_dt所在毫秒
        time_pair = _make_time_pair(begin_ms, end_ms)
        df = FactoryIce.get_enriched_klines_loop(code_id, time_type, time_pair)
        if df.empty:
            return []
        return IceConverter.enriched_df_to_bars(df, code_id, interval_str)

    def _fetch_new_bars(self, code_id: str, interval_str: str, now: datetime) -> list:
        key = (code_id, interval_str)
        begin_ms = self._last_seen_ms[key] + 1
        time_type = getattr(IBTrader.ITimeType, interval_str)
        end_ms = int(now.timestamp() * 1000) + 1
        time_pair = _make_time_pair(begin_ms, end_ms)
        logger.info(
            f"LiveEngine[{self._tag}] _fetch_new_bars {key} thread={threading.current_thread().name} "
            f"begin={datetime.fromtimestamp(begin_ms / 1000):%Y-%m-%d %H:%M:%S} "
            f"end={datetime.fromtimestamp(end_ms / 1000):%Y-%m-%d %H:%M:%S} (now={now:%Y-%m-%d %H:%M:%S})")
        df = FactoryIce.get_enriched_klines_loop(code_id, time_type, time_pair)
        if df.empty:
            return []
        bars = IceConverter.enriched_df_to_bars(df, code_id, interval_str)
        logger.info(
            f"LiveEngine[{self._tag}] _fetch_new_bars {key} -> {len(bars)} bars, "
            f"first={bars[0].datetime}, last={bars[-1].datetime}")
        self._last_seen_ms[key] = int(bars[-1].datetime.timestamp() * 1000)
        return bars

    def _fetch_current_bar(self, code_id: str, interval_str: str, now: datetime) -> list:
        """跟 `_fetch_new_bars` 不同：不依赖 `_last_seen_ms` 游标（无状态，每次都
        只按"当前时刻在这个周期上取整后的bar起始时间"去查，比如 09:49:31 在 M1
        上查 09:49:00 这一根、在 M5 上查 09:45:00 这一根）——哪怕游标因为任何原因
        没推进，请求范围也不会扩大，不会一次性把积压的历史 bar 全部拉回来。
        只给 `poll_all_now()`（固定间隔轮询，不看bar收盘边界）用，不影响
        `poll_once()`/`_fetch_new_bars` 原本的增量游标逻辑。"""
        key = (code_id, interval_str)
        floor_dt = _floor_boundary(interval_str, now)
        begin_ms = int(floor_dt.timestamp() * 1000)
        end_ms = begin_ms + 1
        time_type = getattr(IBTrader.ITimeType, interval_str)
        time_pair = _make_time_pair(begin_ms, end_ms)
        logger.info(
            f"LiveEngine[{self._tag}] _fetch_current_bar {key} thread={threading.current_thread().name} "
            f"floor={floor_dt:%Y-%m-%d %H:%M:%S} (now={now:%Y-%m-%d %H:%M:%S})")
        df = FactoryIce.get_enriched_klines_loop(code_id, time_type, time_pair)
        if df.empty:
            return []
        bars = IceConverter.enriched_df_to_bars(df, code_id, interval_str)
        logger.info(
            f"LiveEngine[{self._tag}] _fetch_current_bar {key} -> {len(bars)} bars, "
            f"first={bars[0].datetime}, last={bars[-1].datetime}")
        return bars

    def poll_once(self, now: datetime = None) -> list:
        """轮询所有到期的 (code_id, interval)，返回本次触发的 PollResult 列表
        （无论有没有拿到新 bar 都会有一条记录——GUI"轮询状态"面板靠这个判断数据
        是否卡住）。调用方负责按 next_wake_time() 控制调用节奏（自己不 sleep）。"""
        now = now or datetime.now()
        due = self.due_keys(now)
        if not due:
            return []

        new_bars: dict = {}
        results = []
        for key in due:
            code_id, interval_str = key
            bars = self._fetch_new_bars(code_id, interval_str, now)
            new_bars[key] = bars
            results.append(PollResult(code_id=code_id, interval=interval_str, poll_time=now,
                                      new_bar_count=len(bars),
                                      latest_bar_dt=bars[-1].datetime if bars else None))
            self._next_wake[key] = self._wake_after(interval_str, now)
            logger.info(f"轮询: {code_id} {interval_str} @ {now:%Y-%m-%d %H:%M:%S} → 新增 {len(bars)} 根bar")

        self._feed_trackers_and_vix(new_bars)
        self._drive_strategies(new_bars)
        return results

    def poll_all_now(self, now: datetime = None) -> None:
        """跟 poll_once() 不同：忽略 bar 收盘边界 + 缓冲秒数的调度（不看
        due_keys()/_next_wake），无条件对所有已注册的 (code_id, interval) 做
        一次"取当前bar+喂tracker+喂策略"。给需要固定间隔轮询（而不是按 bar 收盘
        节奏轮询）的场景用，比如 scripts/signal_monitor.py——不修改、不影响
        poll_once()/due_keys()/next_wake_time() 原本的调度逻辑，`PollWorker`/
        `LiveMainWindow` 这条实盘下单链路完全不受影响。

        取数用 `_fetch_current_bar`（无状态、按取整时间查当前这一根bar），不用
        `_fetch_new_bars`（依赖`_last_seen_ms`游标的增量查询）——同一根bar在
        它所在的那个周期窗口内会被重复取到、重复喂给策略，这是有意为之：
        `ma_approach_monitor`的阈值穿越判断和`uturn_sync_monitor`的
        `_last_alert_dt`去重天然能正确处理"同一根bar反复喂"的情况（close不变
        就不会重复触发），换来的好处是每次请求的时间范围完全由"现在几点"决定、
        不依赖任何持久化游标状态，不可能因为游标没推进而把历史积压一次性拉回来。"""
        now = now or datetime.now()
        logger.info(f"LiveEngine[{self._tag}] poll_all_now 开始 thread={threading.current_thread().name} "
                   f"now={now:%Y-%m-%d %H:%M:%S.%f}")
        new_bars: dict = {}
        for key in self._last_seen_ms:
            code_id, interval_str = key
            new_bars[key] = self._fetch_current_bar(code_id, interval_str, now)
        self._feed_trackers_and_vix(new_bars)
        self._drive_strategies(new_bars)
        logger.info(f"LiveEngine[{self._tag}] poll_all_now 结束 thread={threading.current_thread().name} "
                   f"耗时={(datetime.now() - now).total_seconds():.2f}秒")

    # ── 数据分发 ──────────────────────────────────────────────────────────────

    def _feed_trackers_and_vix(self, new_bars: dict) -> None:
        """先把所有新 bar 喂给相关 tracker/vix_map，必须在标记信号/驱动策略之前
        全部完成——否则同步信号的多品种交集判断会看到不完整的这一轮数据。"""
        for key, bars in new_bars.items():
            if not bars:
                continue
            code_id, interval_str = key
            for strategy in self._vix_targets.get(key, []):
                for bar in bars:
                    if bar.index_valid:
                        strategy._vix_map[bar.datetime] = bar.index_close
            for strategy in self._strategies:
                if strategy.interval.value != interval_str:
                    continue
                for tracker in self._trackers.get(strategy.strategy_id, {}).values():
                    if code_id in tracker.code_ids:
                        for bar in bars:
                            tracker.on_bar(bar)

    def _drive_strategies(self, new_bars: dict) -> None:
        """对每个策略：按时间顺序，把自己品种的新 bar 逐根标记信号（若命中）后
        喂给 on_bar()——emit_intent() 会在这条调用链里被策略内部触发。"""
        for strategy in self._strategies:
            own_bars = new_bars.get((strategy.codeId, strategy.interval.value), [])
            if not own_bars:
                continue
            trackers = self._trackers.get(strategy.strategy_id, {})
            for bar in own_bars:
                if trackers.get('sync') is not None and trackers['sync'].is_signal_now():
                    strategy._sync_timestamps.add(bar.datetime)
                if trackers.get('dip_close') is not None and trackers['dip_close'].is_signal_now():
                    strategy._dip_close_timestamps.add(bar.datetime)
                if trackers.get('dip_reopen') is not None and trackers['dip_reopen'].is_signal_now():
                    strategy._dip_reopen_timestamps.add(bar.datetime)
                strategy.on_bar(bar)
