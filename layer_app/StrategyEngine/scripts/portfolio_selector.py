"""
portfolio_selector.py — $100k 投资组合选择器（离线分析，无需 Ice 服务）

读取 output/ 下所有 UTurnBottomShortPut 优化报告和成交记录，
评分、过滤、去相关后推荐最优品种组合及资金分配。

运行: cd layer_app/StrategyEngine
      PYTHONPATH=ice:src python3 scripts/portfolio_selector.py
"""
import sys
from pathlib import Path

import pandas as pd
import yaml

BASE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(BASE / "src"))

from portfolio_perf import (
    _CORR_GROUPS,
    _assign_corr_group,
    _parse_rank1_metrics,
    _find_opt_files,
    _load_all_metrics,
    _apply_max_dd_floor,
    _estimate_margin,
    _max_concurrent_positions,
    _add_margin_column,
    _print_portfolio,
    _load_multipliers,
    _load_portfolio_deallists,
    _build_equity_curve,
    _build_margin_timeline,
    _calc_max_drawdown,
    _calc_portfolio_sharpe,
    _calc_portfolio_metrics,
    _print_portfolio_metrics,
    _print_open_positions,
    _plot_portfolio,
)


# ─── 评分与过滤 ────────────────────────────────────────────────────────────────

def _score_instruments(df: pd.DataFrame, weights: dict) -> pd.DataFrame:
    df = df.copy()
    w_s = weights.get("sharpe", 1.0)
    w_r = weights.get("recov_factor", 1.0)
    w_t = weights.get("total_return", 10.0)
    df["score"] = w_s * df["sharpe"] + w_r * df["recov_factor"] + w_t * df["total_return"]
    return df


def _apply_filters(df: pd.DataFrame, min_sharpe: float,
                   max_dd_pct: float, min_trades: int,
                   exclude_codes: list[str] | None = None) -> pd.DataFrame:
    max_dd_frac = max_dd_pct / 100
    mask = (
        (df["sharpe"] >= min_sharpe) &
        (df["max_dd"] >= max_dd_frac) &
        (df["total_trades"] >= min_trades)
    )
    if exclude_codes:
        mask &= ~df["code"].isin(exclude_codes)
    return df[mask].copy()


# ─── 组合选择 ──────────────────────────────────────────────────────────────────

def _best_interval_per_code(df: pd.DataFrame) -> pd.DataFrame:
    """每个品种只保留得分最高的时间周期。"""
    return df.loc[df.groupby("code")["score"].idxmax()].reset_index(drop=True)


def _select_portfolio(df: pd.DataFrame, capital: float,
                      max_margin_pct: float, top_n: int,
                      max_total_margin_pct: float = 0.90) -> pd.DataFrame:
    df = df.copy()
    df["group"] = df["code"].map(_assign_corr_group)
    df = df.sort_values("score", ascending=False)
    selected = []
    group_counts: dict[str, int] = {}
    total_margin = 0.0
    for _, row in df.iterrows():
        g = row["group"]
        if group_counts.get(g, 0) >= top_n:
            continue
        if row["peak_margin"] > capital * max_margin_pct:
            continue
        if total_margin + row["peak_margin"] > capital * max_total_margin_pct:
            continue
        selected.append(row)
        group_counts[g] = group_counts.get(g, 0) + 1
        total_margin += row["peak_margin"]
    return pd.DataFrame(selected).reset_index(drop=True)


# ─── 输出 ──────────────────────────────────────────────────────────────────────

def _print_all_candidates(df: pd.DataFrame) -> None:
    disp = df[["code", "interval", "sharpe", "win_rate", "pnl_ratio",
               "max_dd", "recov_factor", "total_return", "total_trades",
               "margin_per_trade", "score"]].copy()
    disp["win_rate"]        = disp["win_rate"].map("{:.1%}".format)
    disp["max_dd"]          = disp["max_dd"].map("{:.2%}".format)
    disp["sharpe"]          = disp["sharpe"].map("{:.2f}".format)
    disp["recov_factor"]    = disp["recov_factor"].map("{:.1f}".format)
    disp["total_return"]    = disp["total_return"].map("{:.1%}".format)
    disp["margin_per_trade"] = disp["margin_per_trade"].map("${:,.0f}".format)
    disp["score"]           = disp["score"].map("{:.1f}".format)
    sep = "═" * 90
    print(f"\n{sep}\n  全量候选（过滤后，每品种取最佳周期，按 score 排序）\n{sep}")
    print(disp.to_string(index=False))


def _print_excluded(candidates: pd.DataFrame, portfolio: pd.DataFrame,
                    capital: float, max_margin_pct: float, top_n: int) -> None:
    """打印被排除的高分候选，标注排除原因（超出保证金 or 组内已有更优选择）。"""
    port_codes = set(portfolio["code"])
    excluded = candidates[~candidates["code"].isin(port_codes)].copy()
    if excluded.empty:
        return
    # 重演选择逻辑，判断排除原因
    group_counts: dict[str, int] = {r["group"]: 0 for _, r in
                                    pd.concat([candidates.assign(
                                        group=candidates["code"].map(_assign_corr_group)),
                                        portfolio]).iterrows()}
    for _, r in portfolio.iterrows():
        g = _assign_corr_group(r["code"])
        group_counts[g] = group_counts.get(g, 0) + 1

    sep = "─" * 90
    print(f"\n{sep}\n  其余高分候选（未入选原因）\n{sep}")
    for _, r in excluded.iterrows():
        g = _assign_corr_group(r["code"])
        pct = r["peak_margin"] / capital * 100
        if r["peak_margin"] > capital * max_margin_pct:
            reason = (f"峰值保证金 ${r['peak_margin']:,.0f}({pct:.0f}%) > 上限 "
                      f"${capital*max_margin_pct:,.0f} → 调 max_margin_pct≥{pct/100:.2f} 可纳入")
        else:
            reason = f"组 {g} 已有更优品种入选（top_n={top_n}）"
        print(f"  {r['code']:6s} {r['interval']:4s} | score:{r['score']:5.0f} "
              f"Sharpe:{r['sharpe']:.2f} | {reason}")
    print()


def _print_portfolio(df: pd.DataFrame, capital: float) -> None:
    total_peak = df["peak_margin"].sum()
    sep = "═" * 90
    print(f"\n{sep}\n  推荐投资组合（按相关性分组去重，$100k 资金）\n{sep}")
    for _, r in df.iterrows():
        alloc_pct = r["peak_margin"] / capital * 100
        print(f"  {r['code']:6s} {r['interval']:4s} | 组别:{r['group']:20s} | "
              f"Sharpe:{r['sharpe']:.2f} WinRate:{r['win_rate']:.1%} MaxDD:{r['max_dd']:.2%} "
              f"RecovF:{r['recov_factor']:.1f} | "
              f"单笔保证金:${r['margin_per_trade']:,.0f} 峰值保证金:${r['peak_margin']:,.0f} ({alloc_pct:.1f}%)")
    print(f"\n  总峰值保证金占用: ${total_peak:,.0f} / $100,000 ({total_peak/capital:.1%})")
    print(f"  闲置缓冲资金:     ${capital - total_peak:,.0f}\n")


def _save_portfolio(df: pd.DataFrame, output_dir: Path) -> None:
    out = output_dir / "portfolio"
    out.mkdir(parents=True, exist_ok=True)
    df.to_csv(out / "recommended.csv", index=False)
    print(f"  结果已保存至 {out}/recommended.csv")


# ─── 主入口 ────────────────────────────────────────────────────────────────────

def _load_config() -> dict:
    path = BASE / "config" / "portfolio_selector.yaml"
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f).get("portfolio", {})


def main() -> None:
    cfg = _load_config()
    capital        = cfg.get("initial_capital", 100_000)
    min_sharpe     = cfg.get("min_sharpe", 2.0)
    max_dd_pct     = cfg.get("max_dd_pct", -5.0)
    max_margin_pct       = cfg.get("max_margin_pct", 0.15)
    max_total_margin_pct = cfg.get("max_total_margin_pct", 0.90)
    top_n                = cfg.get("top_n_per_group", 1)
    min_trades           = cfg.get("min_trades", 20)
    simulate_max1        = cfg.get("simulate_max1", False)

    output_dir   = BASE / "output"
    settings_path = BASE / "config" / "settings.yaml"

    print("正在读取优化报告...")
    df = _load_all_metrics(output_dir)
    df = _apply_max_dd_floor(df)
    print(f"  读取到 {len(df)} 条记录（{df['code'].nunique()} 个品种）")

    if simulate_max1:
        print("  ⚠ simulate_max1=true：保证金按单仓估算（指标仍为旧多仓数据，需重跑 auto_test.py 才准确）")

    recent_n = cfg.get("margin_recent_n", 20)
    multipliers = _load_multipliers(settings_path)
    df = _add_margin_column(df, output_dir, multipliers, simulate_max1, recent_n)
    weights = cfg.get("score_weights", {})
    df = _score_instruments(df, weights)

    exclude_codes = cfg.get("exclude_codes", [])
    filtered = _apply_filters(df, min_sharpe, max_dd_pct, min_trades, exclude_codes)
    print(f"  过滤后剩余 {len(filtered)} 条（Sharpe≥{min_sharpe}, MaxDD≥{max_dd_pct}%, Trades≥{min_trades}）")

    best = _best_interval_per_code(filtered)
    best = best.sort_values("score", ascending=False).reset_index(drop=True)
    _print_all_candidates(best)

    portfolio = _select_portfolio(best, capital, max_margin_pct, top_n, max_total_margin_pct)
    _print_excluded(best, portfolio, capital, max_margin_pct, top_n)
    _print_portfolio(portfolio, capital)
    _save_portfolio(portfolio, output_dir)

    deals = _load_portfolio_deallists(portfolio, output_dir, multipliers)
    if not deals.empty:
        metrics = _calc_portfolio_metrics(deals, capital)
        _print_portfolio_metrics(metrics, capital)
        _print_open_positions(deals, portfolio)
        _plot_portfolio(metrics, capital, output_dir)
    else:
        print("  ⚠ 未找到成交记录，跳过组合绩效计算")


if __name__ == "__main__":
    main()
