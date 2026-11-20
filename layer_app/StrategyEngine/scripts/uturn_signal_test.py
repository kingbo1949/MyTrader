"""
uturn_signal_test.py — 测试 MixUTurnBottom / MixUTurnTop 信号有效性。

信号触发当根 K 线收盘价为入场价，当日最后一根 K 线收盘价为结算价，
计算胜率、平均盈亏点数及 Z-score（二项分布显著性检验）。

运行方式：
    cd layer_app/StrategyEngine
    PYTHONPATH=ice:src python3 scripts/uturn_signal_test.py
"""
import bisect
import logging
import pickle
from collections import deque
from dataclasses import dataclass
from datetime import date, time as dtime, timedelta
from pathlib import Path

import IBTrader
import numpy as np
import pandas as pd
import yaml

from qib_trader.api.converters import IceConverter
from qib_trader.core.models import BarData
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import is_use_cache
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging

logger = logging.getLogger(__name__)

_CONFIG_PATH = Path(__file__).resolve().parents[1] / "config" / "uturn_signal_test.yaml"


# ── 数据结构 ──────────────────────────────────────────────────────────────────

@dataclass(slots=True)
class SettlementResult:
    datetime:         object   # bar 时间
    signal_type:      str      # 'bottom' | 'top'
    signal_direction: int      # +1 | -1
    entry_price:      float
    settle_price:     float
    pnl_points:       float
    is_win:           bool


# ── 配置与数据 ────────────────────────────────────────────────────────────────

def _load_config() -> dict:
    with open(_CONFIG_PATH, encoding="utf-8") as f:
        return yaml.safe_load(f)


def _cache_path(cfg: dict) -> Path:
    d = Path(__file__).resolve().parents[1] / "datadb"
    d.mkdir(exist_ok=True)
    start = cfg["start_time"][:10].replace("-", "")
    end   = cfg["end_time"][:10].replace("-", "")
    return d / f"{cfg['code_id']}_{cfg['interval']}_{start}_{end}.pkl"


def _fetch_bars(cfg: dict) -> list[BarData]:
    cache = _cache_path(cfg)
    if is_use_cache() and cache.exists():
        logger.info(f"从本地缓存加载: {cache}")
        with open(cache, "rb") as f:
            return pickle.load(f)

    time_type = getattr(IBTrader.ITimeType, cfg["interval"])
    time_pair = FactoryIce.make_timepair(cfg["start_time"], cfg["end_time"])
    df = FactoryIce.get_enriched_klines_loop(cfg["code_id"], time_type, time_pair)
    if df.empty:
        logger.warning(f"未获取到数据: {cfg['code_id']} {cfg['interval']}")
        return []
    bars = IceConverter.enriched_df_to_bars(df, cfg["code_id"], cfg["interval"])
    with open(cache, "wb") as f:
        pickle.dump(bars, f)
    logger.info(f"数据已缓存到: {cache}")
    return bars


def _build_session_closes(bars: list[BarData], close_time_str: str) -> tuple[list, list]:
    """返回 (datetimes, prices)，每对对应一个交易日收盘（时间 ≤ market_close_time 的最后一根 bar）。"""
    h, m, s = map(int, close_time_str.split(":"))
    ct = dtime(h, m, s)
    daily: dict[date, BarData] = {}
    for bar in bars:
        if bar.datetime.time() <= ct:
            d = bar.datetime.date()
            if d not in daily or bar.datetime > daily[d].datetime:
                daily[d] = bar
    closes = sorted(daily.values(), key=lambda b: b.datetime)
    return [b.datetime for b in closes], [b.close_price for b in closes]


# ── 信号扫描 ──────────────────────────────────────────────────────────────────

def _is_plus_pattern(zone) -> bool:
    """当前 UTurn 点的 |dif| 大于同方向前一个 UTurn 点的 |dif|。"""
    if zone.zone_type == 'bottom':
        uturn_bars = [b for b in zone.zone_bars if b.div_type in ('Bottom', 'BottomSub') and b.is_uturn]
    else:
        uturn_bars = [b for b in zone.zone_bars if b.div_type in ('Top', 'TopSub') and b.is_uturn]
    if len(uturn_bars) < 2:
        return False
    return abs(uturn_bars[-1].dif) > abs(uturn_bars[-2].dif)


def _detect_signal(window: deque, bar: BarData, cfg: dict) -> int:
    """
    返回 +1（MixUTurnBottom）、-1（MixUTurnTop）或 0（无信号）。
    根据 cfg 中的过滤条件决定是否通过。
    """
    zone = detect_multi_uturn(list(window))
    if zone is None or not zone.is_mix:
        return 0

    require_plus   = cfg.get("require_plus_pattern", 0)
    req_low        = cfg.get("require_low_below_prev_day_low", 0)
    req_high       = cfg.get("require_high_above_prev_day_high", 0)

    if zone.zone_type == 'bottom':
        if req_low and bar.prev_day_low > 0 and bar.low_price >= bar.prev_day_low:
            return 0
        if require_plus and not _is_plus_pattern(zone):
            return 0
        return 1

    if zone.zone_type == 'top':
        if req_high and bar.prev_day_high > 0 and bar.high_price <= bar.prev_day_high:
            return 0
        if require_plus and not _is_plus_pattern(zone):
            return 0
        return -1

    return 0


def _is_too_close_to_close(bar: BarData, min_minutes: int, close_time_str: str) -> bool:
    """当前 bar 时间距当日收盘不足 min_minutes 分钟时返回 True。"""
    if min_minutes <= 0:
        return False
    h, m, s = map(int, close_time_str.split(":"))
    close_dt = bar.datetime.replace(hour=h, minute=m, second=s, microsecond=0)
    return bar.datetime >= close_dt - timedelta(minutes=min_minutes)


def _is_in_valid_time(bar: BarData, valid_times: list) -> bool:
    """bar 时间是否落在任意一个有效时间段内；valid_times 为空则全天有效。"""
    if not valid_times:
        return True
    t = bar.datetime.time()
    for start_str, end_str in valid_times:
        sh, sm, ss = map(int, start_str.split(":"))
        eh, em, es = map(int, end_str.split(":"))
        if dtime(sh, sm, ss) <= t <= dtime(eh, em, es):
            return True
    return False


def _run_signal_scan(bars: list[BarData], cfg: dict) -> list[SettlementResult]:
    """逐 bar 扫描信号，结合下一个收盘价构建 SettlementResult 列表。"""
    close_time    = cfg.get("market_close_time", "16:00:00")
    sc_dts, sc_prices = _build_session_closes(bars, close_time)
    window: deque = deque(maxlen=cfg.get("multi_uturn_window", 300))
    cooldown_bars = cfg.get("cooldown_bars", 0)
    min_minutes   = cfg.get("min_minutes_to_close", 0)
    valid_times   = cfg.get("signal_valid_times", [])
    last_signal_bar: dict[int, int] = {1: -999999, -1: -999999}
    results: list[SettlementResult] = []

    for i, bar in enumerate(bars):
        window.append(bar)
        if not _is_in_valid_time(bar, valid_times):
            continue
        if _is_too_close_to_close(bar, min_minutes, close_time):
            continue
        signal = _detect_signal(window, bar, cfg)
        if signal == 0:
            continue
        if cooldown_bars > 0 and i - last_signal_bar[signal] < cooldown_bars:
            continue
        idx = bisect.bisect_right(sc_dts, bar.datetime)
        if idx >= len(sc_dts):
            continue
        settle = sc_prices[idx]
        last_signal_bar[signal] = i
        pnl = (settle - bar.close_price) * signal
        results.append(SettlementResult(
            datetime=bar.datetime,
            signal_type='bottom' if signal == 1 else 'top',
            signal_direction=signal,
            entry_price=bar.close_price,
            settle_price=settle,
            pnl_points=pnl,
            is_win=pnl > 0,
        ))

    return results


# ── 统计分析 ──────────────────────────────────────────────────────────────────

def _analyze(results: list[SettlementResult], signal_type: str) -> dict:
    subset = [r for r in results if r.signal_type == signal_type]
    if not subset:
        return {"signal_type": signal_type, "status": "无信号"}

    wins      = sum(1 for r in subset if r.is_win)
    total     = len(subset)
    win_rate  = wins / total
    avg_pnl   = sum(r.pnl_points for r in subset) / total
    avg_win   = sum(r.pnl_points for r in subset if r.is_win) / wins if wins else 0.0
    avg_loss  = sum(r.pnl_points for r in subset if not r.is_win) / (total - wins) if total - wins else 0.0
    z_score   = (wins - total * 0.5) / (np.sqrt(total) * 0.5)
    expectancy = win_rate * avg_win + (1 - win_rate) * avg_loss

    return {
        "signal_type":     signal_type,
        "total_signals":   total,
        "win_rate":        f"{win_rate:.2%}",
        "avg_pnl_points":  round(avg_pnl, 2),
        "avg_win_points":  round(avg_win, 2),
        "avg_loss_points": round(avg_loss, 2),
        "expectancy":      round(expectancy, 2),
        "z_score":         round(z_score, 2),
    }


# ── 输出 ──────────────────────────────────────────────────────────────────────

def _output_dir(code_id: str) -> Path:
    d = Path(__file__).resolve().parents[1] / "output" / code_id / "signal_test"
    d.mkdir(parents=True, exist_ok=True)
    return d


def _export_csv(results: list[SettlementResult], code_id: str, interval: str) -> None:
    from dataclasses import asdict
    df = pd.DataFrame([asdict(r) for r in results])
    path = _output_dir(code_id) / f"uturn_signal_test_{code_id}_{interval}.csv"
    df.to_csv(path, index=False, float_format="%.4f")
    logger.info(f"明细已导出: {path}")


def _print_summary(stats_bottom: dict, stats_top: dict) -> None:
    for stats in (stats_bottom, stats_top):
        logger.info("-" * 50)
        for k, v in stats.items():
            logger.info(f"  {k:<20}: {v}")
    logger.info("-" * 50)


# ── 主入口 ────────────────────────────────────────────────────────────────────

def main() -> None:
    setup_logging()
    cfg  = _load_config()
    bars = _fetch_bars(cfg)
    if not bars:
        return

    logger.info(f"共 {len(bars)} 根 bar，开始扫描信号...")
    results = _run_signal_scan(bars, cfg)
    logger.info(f"触发信号总计 {len(results)} 次")

    stats_bottom = _analyze(results, 'bottom')
    stats_top    = _analyze(results, 'top')
    _print_summary(stats_bottom, stats_top)
    _export_csv(results, cfg["code_id"], cfg["interval"])


if __name__ == "__main__":
    try:
        main()
    finally:
        FactoryIce.shutdown()
