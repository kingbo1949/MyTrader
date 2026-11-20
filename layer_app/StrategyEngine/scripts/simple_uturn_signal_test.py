"""
simple_uturn_signal_test.py — 测试简单 UTurnBottom / UTurnTop 信号有效性。

信号条件：bar.is_uturn=True 且 abs(bar.dif) >= min_dif_abs
  UTurnBottom → 做多目标品种
  UTurnTop    → 做空目标品种

信号触发当根 K 线收盘价为入场价，当日最后一根 K 线收盘价为结算价，
计算胜率、平均盈亏点数及 Z-score（二项分布显著性检验）。

运行方式：
    cd layer_app/StrategyEngine
    PYTHONPATH=ice:src python3 scripts/simple_uturn_signal_test.py
"""
import bisect
import logging
import pickle
from dataclasses import dataclass, asdict
from datetime import date, datetime, time as dtime, timedelta
from pathlib import Path

import IBTrader
import numpy as np
import pandas as pd
import yaml

from qib_trader.api.converters import IceConverter
from qib_trader.core.models import BarData
from qib_trader.utils.config_loader import is_use_cache
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging

logger = logging.getLogger(__name__)

_CONFIG_PATH = Path(__file__).resolve().parents[1] / "config" / "simple_uturn_signal_test.yaml"


# ── 数据结构 ──────────────────────────────────────────────────────────────────

@dataclass(slots=True)
class SettlementResult:
    datetime:         object
    signal_type:      str    # 'bottom' | 'top'
    signal_direction: int    # +1（做多）| -1（做空）
    dif:              float
    entry_price:      float
    settle_price:     float
    pnl_points:       float
    is_win:           bool
    exit_type:        str    # 'settle' | 'stop'
    rvold:            float  # 信号触发时成交量相对过去N根bar的Z-score


@dataclass(slots=True)
class _PendingPos:
    entry_dt:    object
    entry_price: float
    direction:   int
    expiry_dt:   object
    dif:         float
    rvold:       float = 0.0


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
    """返回 (datetimes, prices)，每对对应一个交易日的收盘（时间 ≤ market_close_time 的最后一根 bar）。
    用 bisect 在信号时间之后找下一个收盘，解决盘后信号错误结算到当日收盘（信号之前）的问题。
    """
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


def _cap_to_friday_close(bar_dt: datetime, expiry_dt: datetime, close_time: dtime) -> datetime:
    """若 bar_dt → expiry_dt 跨越周末（存在周六），返回该周六前一天周五的收盘时刻。"""
    d = bar_dt.date() + timedelta(days=1)
    while d <= expiry_dt.date():
        if d.weekday() == 5:  # 周六
            friday = datetime.combine(d - timedelta(days=1), close_time)
            return friday if friday > bar_dt else expiry_dt
        d += timedelta(days=1)
    return expiry_dt


def _calc_expiry_dt(bar_dt: datetime, option_days: int, expiry_weekday: int,
                    close_time: dtime) -> datetime:
    """计算结算时刻。
    - 先以 bar_dt + option_days 天为基准，按 expiry_weekday 找到期日
    - 若到期时刻已在 bar_dt 之前或相等（市场已收盘），顺延一个周期：
        expiry_weekday=-1 → 顺延到下一个交易日
        expiry_weekday>=0 → 顺延到下一周同一星期几
    """
    target = bar_dt + timedelta(days=option_days)
    if expiry_weekday == -1:
        extra = {5: 2, 6: 1}.get(target.weekday(), 0)
        expiry_date = (target + timedelta(days=extra)).date()
    else:
        days_ahead = (expiry_weekday - target.weekday()) % 7
        expiry_date = (target + timedelta(days=days_ahead)).date()
    expiry_dt = datetime.combine(expiry_date, close_time)
    if expiry_dt <= bar_dt:
        if expiry_weekday == -1:
            nxt = expiry_dt + timedelta(days=1)
            extra = {5: 2, 6: 1}.get(nxt.weekday(), 0)
            expiry_date = (nxt + timedelta(days=extra)).date()
        else:
            expiry_date = expiry_date + timedelta(days=7)
        expiry_dt = datetime.combine(expiry_date, close_time)
    return expiry_dt


# ── 信号检测 ──────────────────────────────────────────────────────────────────

def _calc_rvold(bars: list[BarData], i: int, lookback: int) -> float:
    """当前成交量相对过去 lookback 根 bar 的 Z-score。"""
    vols = [b.volume for b in bars[max(0, i - lookback):i]]
    if len(vols) < 2:
        return 0.0
    v_std = float(np.std(vols))
    if v_std == 0:
        return 0.0
    return (bars[i].volume - float(np.mean(vols))) / v_std


_SIGNAL_DIRECTION: dict[str, int] = {
    'bottom':  1,
    'top':    -1,
}


def _detect_signal(bar: BarData, signal_cfg: dict) -> bool:
    """判断 bar 是否触发 signal_cfg 指定类型的信号。"""
    if not bar.is_uturn:
        return False
    if abs(bar.dif) < signal_cfg.get("min_dif_abs", 0.0):
        return False
    sig_type = signal_cfg["type"]
    if sig_type == "bottom":
        if bar.div_type not in ('Bottom', 'BottomSub'):
            return False
        req_low = signal_cfg.get("require_low_below_prev_day_low", 0)
        return not (req_low and bar.prev_day_low > 0 and bar.low_price >= bar.prev_day_low)
    if sig_type == "top":
        if bar.div_type not in ('Top', 'TopSub'):
            return False
        req_high = signal_cfg.get("require_high_above_prev_day_high", 0)
        return not (req_high and bar.prev_day_high > 0 and bar.high_price <= bar.prev_day_high)
    return False


def _is_too_close_to_close(bar: BarData, min_minutes: int, close_time_str: str) -> bool:
    if min_minutes <= 0:
        return False
    h, m, s = map(int, close_time_str.split(":"))
    close_dt = bar.datetime.replace(hour=h, minute=m, second=s, microsecond=0)
    return bar.datetime >= close_dt - timedelta(minutes=min_minutes)


def _is_in_valid_time(bar: BarData, valid_times: list) -> bool:
    if not valid_times:
        return True
    t = bar.datetime.time()
    for start_str, end_str in valid_times:
        sh, sm, ss = map(int, start_str.split(":"))
        eh, em, es = map(int, end_str.split(":"))
        if dtime(sh, sm, ss) <= t <= dtime(eh, em, es):
            return True
    return False


def _is_after_friday_close(bar: BarData, close_time_str: str) -> bool:
    """判断 bar 是否为周五收盘时刻或之后（此时开仓会持仓过周末，应丢弃）。"""
    if bar.datetime.weekday() != 4:
        return False
    h, m, s = map(int, close_time_str.split(":"))
    close_dt = bar.datetime.replace(hour=h, minute=m, second=s, microsecond=0)
    return bar.datetime >= close_dt


def _check_stop(bar: BarData, pos: _PendingPos, stop_pts: float) -> bool:
    if stop_pts <= 0:
        return False
    if pos.direction == 1:
        return bar.low_price < pos.entry_price - stop_pts
    return bar.high_price > pos.entry_price + stop_pts


# ── 扫描 ──────────────────────────────────────────────────────────────────────

def _run_signal_scan(bars: list[BarData], global_cfg: dict, signal_cfg: dict) -> list[SettlementResult]:
    close_time     = global_cfg.get("market_close_time", "16:00:00")
    sc_dts, sc_prices = _build_session_closes(bars, close_time)
    option_days    = global_cfg.get("option_days", 0)
    expiry_weekday = global_cfg.get("expiry_weekday", -1)
    no_hold_weekend = global_cfg.get("no_hold_weekend", False)
    cooldown_bars  = signal_cfg.get("cooldown_bars", 0)
    rvold_lookback = signal_cfg.get("rvold_lookback", 20)
    min_minutes    = signal_cfg.get("min_minutes_to_close", 0)
    valid_times    = signal_cfg.get("signal_valid_times", [])
    stop_loss_pts  = signal_cfg.get("stop_loss_points", 0)
    sig_type       = signal_cfg["type"]
    direction      = _SIGNAL_DIRECTION[sig_type]
    last_signal_bar = -999999
    results: list[SettlementResult] = []
    pending: list[_PendingPos] = []

    h, m, s = map(int, close_time.split(":"))
    ct = dtime(h, m, s)

    for i, bar in enumerate(bars):
        still_open: list[_PendingPos] = []
        for pos in pending:
            if _check_stop(bar, pos, stop_loss_pts):
                settle_px = pos.entry_price - stop_loss_pts * pos.direction
                results.append(SettlementResult(
                    datetime=pos.entry_dt, signal_type=sig_type,
                    signal_direction=pos.direction, dif=pos.dif,
                    entry_price=pos.entry_price, settle_price=settle_px,
                    pnl_points=-stop_loss_pts, is_win=False, exit_type='stop',
                    rvold=pos.rvold,
                ))
                last_signal_bar = max(last_signal_bar, i)
            elif bar.datetime >= pos.expiry_dt:
                idx = bisect.bisect_left(sc_dts, pos.expiry_dt)
                if idx < len(sc_dts):
                    settle = sc_prices[idx]
                    pnl = (settle - pos.entry_price) * pos.direction
                    results.append(SettlementResult(
                        datetime=pos.entry_dt, signal_type=sig_type,
                        signal_direction=pos.direction, dif=pos.dif,
                        entry_price=pos.entry_price, settle_price=settle,
                        pnl_points=pnl, is_win=pnl > 0, exit_type='settle',
                        rvold=pos.rvold,
                    ))
            else:
                still_open.append(pos)
        pending = still_open

        if not _is_in_valid_time(bar, valid_times):
            continue
        if _is_too_close_to_close(bar, min_minutes, close_time):
            continue
        if no_hold_weekend and _is_after_friday_close(bar, close_time):
            continue
        if not _detect_signal(bar, signal_cfg):
            continue
        if cooldown_bars > 0 and i - last_signal_bar < cooldown_bars:
            continue
        expiry_dt = _calc_expiry_dt(bar.datetime, option_days, expiry_weekday, ct)
        if no_hold_weekend:
            expiry_dt = _cap_to_friday_close(bar.datetime, expiry_dt, ct)
        last_signal_bar = i
        pending.append(_PendingPos(
            entry_dt=bar.datetime, entry_price=bar.close_price,
            direction=direction, expiry_dt=expiry_dt, dif=bar.dif,
            rvold=_calc_rvold(bars, i, rvold_lookback),
        ))

    return results


# ── 统计分析 ──────────────────────────────────────────────────────────────────

def _analyze(results: list[SettlementResult], signal_type: str) -> dict:
    if not results:
        return {"signal_type": signal_type, "status": "无信号"}

    wins       = sum(1 for r in results if r.is_win)
    total      = len(results)
    win_rate   = wins / total
    avg_pnl    = sum(r.pnl_points for r in results) / total
    avg_win    = sum(r.pnl_points for r in results if r.is_win) / wins if wins else 0.0
    avg_loss   = sum(r.pnl_points for r in results if not r.is_win) / (total - wins) if total - wins else 0.0
    z_score    = (wins - total * 0.5) / (np.sqrt(total) * 0.5)
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


def _export_csv(results: list[SettlementResult], code_id: str, interval: str, signal_type: str) -> None:
    df = pd.DataFrame([asdict(r) for r in results])
    path = _output_dir(code_id) / f"simple_uturn_signal_test_{code_id}_{interval}_{signal_type}.csv"
    df.to_csv(path, index=False, float_format="%.4f")
    logger.info(f"明细已导出: {path}")


def _print_summary(stats_list: list[dict]) -> None:
    for stats in stats_list:
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
    enabled = [s for s in cfg.get("signals", []) if s.get("enable", True)]
    stats_list = []
    for signal_cfg in enabled:
        sig_type = signal_cfg["type"]
        results  = _run_signal_scan(bars, cfg, signal_cfg)
        logger.info(f"[{sig_type}] 触发信号 {len(results)} 次")
        stats_list.append(_analyze(results, sig_type))
        _export_csv(results, cfg["code_id"], cfg["interval"], sig_type)
    _print_summary(stats_list)


if __name__ == "__main__":
    try:
        main()
    finally:
        FactoryIce.shutdown()
