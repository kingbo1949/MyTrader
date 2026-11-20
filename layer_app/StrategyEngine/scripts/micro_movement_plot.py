"""
micro_movement_plot.py — 信号后微观价格路径分析

对每个信号触发后 LOOKBACK_BARS 根 bar 提取逐 bar 收盘价相对入场价的位移，
绘制均值路径曲线 + ±1σ 带，并输出关键时间点统计。

支持与 signal_character_plot.py 相同的三种数据源：
  "signal_test"       全量信号（simple_uturn_signal_test.py 输出）
  "lead_lag"          同步子集（lead_lag_analysis.py 输出）
  "closed_positions"  回测成交明细（auto_test / main_backtest 输出）

运行方式：
    cd layer_app/StrategyEngine
    PYTHONPATH=ice:src python3 scripts/micro_movement_plot.py
"""
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from signal_character_plot import (
    _load_yaml_config,
    _load_price_df,
    _load_from_signal_test,
    _load_from_lead_lag,
    _load_from_closed_positions,
)

_BASE = Path(__file__).resolve().parents[1]

# ── 硬编码运行配置（按需修改）────────────────────────────────────────────────

SOURCE      = "lead_lag"   # "signal_test" | "lead_lag" | "closed_positions"
SIGNAL_TYPE = "bottom"             # "bottom" | "top"（signal_test 模式使用）

PRICE_INTERVAL = "M5"    # 微观分析所用 K线周期
LOOKBACK_BARS  = 48      # 提取多少根 bar（48 × M5 = 4 小时）

# lead_lag 模式
LEAD_LAG_FILE = _BASE / "output" / "lead_lag_NQ_ES_bottom_M1.csv"
INSTRUMENT    = "ES"            # "lead" / "NQ" 或 lag 品种名（如 "ES"）
FILTER_MODE   = "exact_sync"    # "all" | "synced" | "exact_sync"
LAG_IDS       = ["ES"]

# closed_positions 模式
CLOSED_POSITIONS_FILE    = _BASE / "output" / "NQ" / "deallist" / "UTurnBSync_DebitCallSpread_NQ_M5_LONG.csv"
CLOSED_POSITIONS_CODE_ID = "NQ"


# ── 工具 ──────────────────────────────────────────────────────────────────────

def _interval_minutes(interval: str) -> int:
    s = interval.upper()
    n = int(s[1:])
    return n * 60 if s.startswith("H") else n


# ── 核心计算 ──────────────────────────────────────────────────────────────────

def _extract_paths(signals_df: pd.DataFrame, price_df: pd.DataFrame,
                   lookback_bars: int, direction: int) -> pd.DataFrame:
    """每个信号 → 入场后 lookback_bars 根 bar 的逐 bar 位移宽表。"""
    price_df = price_df.sort_values("datetime").reset_index(drop=True)
    records = []
    for _, sig in signals_df.iterrows():
        t0, p0 = sig["entry_time"], sig["entry_price"]
        future = price_df[price_df["datetime"] >= t0].head(lookback_bars)
        if len(future) < lookback_bars:
            continue
        row = {"entry_time": t0}
        for j, close in enumerate(future["close"].values, start=1):
            row[f"bar{j}"] = (close - p0) * direction
        records.append(row)
    return pd.DataFrame(records)


def _compute_stats(paths_df: pd.DataFrame, bar_min: int) -> dict:
    bar_cols = [c for c in paths_df.columns if c.startswith("bar")]
    data = paths_df[bar_cols]
    checkpoints = {}
    for minutes in [30, 60, 120, 240]:
        idx = minutes // bar_min - 1
        col = f"bar{idx + 1}"
        checkpoints[f"T+{minutes}min"] = data[col].mean() if col in data.columns else None
    return {
        **checkpoints,
        "MaxRunup_avg":    data.max(axis=1).mean(),
        "MaxDrawdown_avg": data.min(axis=1).mean(),
        "n_signals":       len(paths_df),
    }


# ── 可视化 ────────────────────────────────────────────────────────────────────

def _plot(paths_df: pd.DataFrame, stats: dict, code_id: str,
          signal_type: str, source_label: str, bar_min: int) -> None:
    bar_cols = [c for c in paths_df.columns if c.startswith("bar")]
    data = paths_df[bar_cols].values
    x    = [i * bar_min for i in range(1, len(bar_cols) + 1)]
    mean = np.mean(data, axis=0)
    std  = np.std(data, axis=0)

    fig, ax = plt.subplots(figsize=(14, 6))
    ax.axhline(0, color="black", linewidth=0.8, zorder=1)
    ax.fill_between(x, mean - std, mean + std, alpha=0.18, color="steelblue", label="±1σ")
    ax.plot(x, mean, color="steelblue", linewidth=2, label="Mean path")

    # 关键时间点标注
    for minutes in [30, 60, 120, 240]:
        avg = stats.get(f"T+{minutes}min")
        if avg is None:
            continue
        ax.axvline(minutes, color="gray", linestyle=":", linewidth=0.8)
        ax.annotate(f"{avg:+.1f}pt",
                    xy=(minutes, avg), xytext=(minutes + bar_min, avg),
                    fontsize=8, color="dimgray")

    info = (f"n={stats['n_signals']}  "
            f"MaxRunup={stats['MaxRunup_avg']:+.1f}pt  "
            f"MaxDD={stats['MaxDrawdown_avg']:+.1f}pt")
    ax.set_title(f"{code_id} [{signal_type}]  {source_label}\n{info}")
    ax.set_xlabel(f"Minutes after entry  (1 bar = {bar_min} min)")
    ax.set_ylabel("Price change from entry (points)")
    ax.legend()
    ax.grid(True, alpha=0.3)

    out_dir   = _BASE / "output" / code_id / "signal_test"
    out_dir.mkdir(parents=True, exist_ok=True)
    safe_label = source_label.replace(" ", "_").replace("/", "_")
    save_path  = out_dir / f"micro_movement_{code_id}_{signal_type}_{safe_label}.png"
    fig.savefig(save_path, dpi=150, bbox_inches="tight")
    plt.show()
    print(f"图表已保存: {save_path}")
    _print_stats(stats)


def _print_stats(stats: dict) -> None:
    print(f"\n{'─' * 40}")
    for k, v in stats.items():
        if v is None:
            continue
        print(f"  {k:<22}: {v:+.2f}" if isinstance(v, float) else f"  {k:<22}: {v}")
    print(f"{'─' * 40}\n")


# ── 主入口 ────────────────────────────────────────────────────────────────────

def main() -> None:
    cfg = _load_yaml_config()

    if SOURCE == "signal_test":
        signals_df, code_id, direction = _load_from_signal_test(cfg, SIGNAL_TYPE)
        source_label = "full"
    elif SOURCE == "lead_lag":
        signals_df, code_id, direction = _load_from_lead_lag(
            LEAD_LAG_FILE, INSTRUMENT, FILTER_MODE, LAG_IDS)
        instr_label  = "NQ" if INSTRUMENT in ("lead", "NQ") else INSTRUMENT
        source_label = f"{instr_label}_{FILTER_MODE}"
    else:  # closed_positions
        signals_df, code_id, direction = _load_from_closed_positions(
            CLOSED_POSITIONS_FILE, CLOSED_POSITIONS_CODE_ID)
        source_label = "backtest"

    print(f"[{code_id}] 加载 {len(signals_df)} 条记录（{source_label}）")
    price_df = _load_price_df(code_id, PRICE_INTERVAL, cfg)

    bar_min  = _interval_minutes(PRICE_INTERVAL)
    paths_df = _extract_paths(signals_df, price_df, LOOKBACK_BARS, direction)
    if paths_df.empty:
        print("数据不足，无法生成路径图（信号后 bar 数不够）")
        return

    stats = _compute_stats(paths_df, bar_min)
    _plot(paths_df, stats, code_id, SIGNAL_TYPE, source_label, bar_min)


if __name__ == "__main__":
    main()
