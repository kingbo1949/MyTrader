"""
signal_pnl_plot.py — 对 simple_uturn_signal_test 输出的 CSV 进行盈亏分布分析。
可选叠加 lead_lag_analysis 输出的完全同步子集，对比全量与同步集合的差异。

运行方式：
    cd layer_app/StrategyEngine
    python3 scripts/signal_pnl_plot.py
"""
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import scipy.stats as stats
import seaborn as sns

_OUTPUT_ROOT = Path(__file__).resolve().parents[1] / "output"


# ── 数据加载 ──────────────────────────────────────────────────────────────────

def _load(code_id: str, interval: str, signal_type: str) -> pd.DataFrame:
    path = _OUTPUT_ROOT / code_id / "signal_test" / f"simple_uturn_signal_test_{code_id}_{interval}_{signal_type}.csv"
    if not path.exists():
        raise FileNotFoundError(f"找不到文件: {path}\n先运行 simple_uturn_signal_test.py 生成数据。")
    return pd.read_csv(path)


def _load_exact_sync(sync_ids: list, interval: str, signal_type: str) -> dict:
    """从 lead_lag CSV 中提取完全同步子集，返回 {code_id: pnl_series}。"""
    label = "_".join(sync_ids)
    path  = _OUTPUT_ROOT / f"lead_lag_{label}_{signal_type}_{interval}.csv"
    if not path.exists():
        raise FileNotFoundError(f"找不到: {path}\n先运行 lead_lag_analysis.py 生成数据。")
    df      = pd.read_csv(path)
    lag_ids = sync_ids[1:]
    mask    = df["is_synced"] == True
    for lag_id in lag_ids:
        mask &= df[f"{lag_id}_diff_mins"] == 0
    exact  = df[mask]
    result = {sync_ids[0]: exact["lead_pnl"].reset_index(drop=True)}
    for lag_id in lag_ids:
        result[lag_id] = exact[f"{lag_id}_pnl"].reset_index(drop=True)
    return result


# ── 统计摘要 ──────────────────────────────────────────────────────────────────

def _print_stats(pnl: pd.Series, label: str) -> None:
    wins  = (pnl > 0).sum()
    total = len(pnl)
    print(f"\n{'─'*50}")
    print(f"  信号类型   : {label}  (n={total})")
    print(f"  胜率       : {wins/total:.2%}")
    print(f"  期望值     : {pnl.mean():.2f}")
    print(f"  平均盈利   : {pnl[pnl>0].mean():.2f}")
    print(f"  平均亏损   : {pnl[pnl<=0].mean():.2f}")
    print(f"  中位数     : {pnl.median():.2f}")
    print(f"  偏度       : {pnl.skew():.2f}  (正值=右尾更厚)")
    print(f"  峰度       : {pnl.kurt():.2f}  (正值=厚尾极端事件多)")
    z = (wins - total * 0.5) / (np.sqrt(total) * 0.5)
    print(f"  Z-score    : {z:.2f}")


# ── 图表绘制 ──────────────────────────────────────────────────────────────────

def _plot_distribution(ax: plt.Axes, pnl: pd.Series, label: str) -> None:
    sns.histplot(pnl, kde=True, bins=80, color="royalblue",
                 edgecolor="black", alpha=0.6, ax=ax)
    ax.axvline(0,             color="black",  linewidth=1.5)
    ax.axvline(pnl.mean(),    color="red",    linestyle="--", linewidth=2,
               label=f"Expectancy: {pnl.mean():.2f}")
    ax.axvline(pnl[pnl>0].mean(),  color="green",  linestyle=":", linewidth=2,
               label=f"Avg Win: {pnl[pnl>0].mean():.2f}")
    ax.axvline(pnl[pnl<=0].mean(), color="orange", linestyle=":", linewidth=2,
               label=f"Avg Loss: {pnl[pnl<=0].mean():.2f}")
    ax.set_title(f"PnL Distribution — {label}", fontsize=13)
    ax.set_xlabel("PnL Points")
    ax.set_ylabel("Frequency")
    ax.legend()


def _plot_qq(ax: plt.Axes, pnl: pd.Series, label: str) -> None:
    (osm, osr), (slope, intercept, r) = stats.probplot(pnl, dist="norm")
    ax.scatter(osm, osr, s=8, alpha=0.5, color="royalblue")
    x_line = np.array([osm[0], osm[-1]])
    ax.plot(x_line, slope * x_line + intercept, color="red", linewidth=1.5)
    ax.set_title(f"Normal Q-Q Plot — {label}  (R={r:.3f})", fontsize=13)
    ax.set_xlabel("Theoretical Quantiles")
    ax.set_ylabel("Sample Quantiles")


def _plot_signal(df: pd.DataFrame, label: str) -> None:
    pnl = df["pnl_points"].dropna()
    if pnl.empty:
        print(f"[跳过] {label}: 无数据")
        return
    _print_stats(pnl, label)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle(label, fontsize=14)
    _plot_distribution(ax1, pnl, label)
    _plot_qq(ax2, pnl, label)
    plt.tight_layout()


def _plot_comparison(pnl_full: pd.Series, pnl_sync: pd.Series, label: str) -> None:
    """左图：全量与同步集合分布叠加；右图：同步集合 Q-Q 图。"""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle(label, fontsize=14)

    # 分布叠加
    sns.histplot(pnl_full, kde=True, bins=80, color="royalblue",
                 alpha=0.35, label=f"全量 (n={len(pnl_full)})", ax=ax1)
    sns.histplot(pnl_sync, kde=True, bins=60, color="darkorange",
                 alpha=0.45, label=f"exact_sync (n={len(pnl_sync)})", ax=ax1)
    ax1.axvline(0, color="black", linewidth=1.5)
    ax1.axvline(pnl_full.mean(), color="royalblue", linestyle="--", linewidth=2,
                label=f"全量期望: {pnl_full.mean():.2f}")
    ax1.axvline(pnl_sync.mean(), color="darkorange", linestyle="--", linewidth=2,
                label=f"同步期望: {pnl_sync.mean():.2f}")
    ax1.set_title(f"PnL Distribution — {label}", fontsize=13)
    ax1.set_xlabel("PnL Points")
    ax1.set_ylabel("Frequency")
    ax1.legend()

    # 同步集合 Q-Q
    _plot_qq(ax2, pnl_sync, f"{label} exact_sync")
    plt.tight_layout()


# ── 主入口 ────────────────────────────────────────────────────────────────────

def main() -> None:
    code_id     = "SMH"
    interval    = "M1"
    signal_type = "bottom"
    # 同步集合配置：与哪些品种做完全同步过滤（顺序须与 lead_lag_analysis.py 一致）
    # 设为 None 则只画全量曲线
    sync_ids    = ["NQ", "ES", "SMH"]
    # sync_ids = []

    df       = _load(code_id, interval, signal_type)
    pnl_full = df["pnl_points"].dropna()

    sns.set_theme(style="whitegrid")

    if sync_ids:
        synced_pnls = _load_exact_sync(sync_ids, interval, signal_type)
        if code_id not in synced_pnls:
            raise ValueError(f"{code_id} 不在 sync_ids {sync_ids} 中")
        pnl_sync = synced_pnls[code_id].dropna()
        _print_stats(pnl_full, f"{code_id} 全量")
        _print_stats(pnl_sync, f"{code_id} exact_sync")
        _plot_comparison(pnl_full, pnl_sync, f"{code_id} {interval} {signal_type}")
    else:
        _plot_signal(df, f"{code_id} {interval} — {signal_type}")

    plt.show()


if __name__ == "__main__":
    main()
