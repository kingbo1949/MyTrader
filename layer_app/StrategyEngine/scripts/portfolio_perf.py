"""
portfolio_perf.py — 组合绩效计算共用模块

被 portfolio_selector.py 和 portfolio_analyzer.py 共同使用。
"""
import re
from pathlib import Path

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import pandas as pd
import yaml

BASE = Path(__file__).resolve().parents[1]

# ─── 相关性分组 ────────────────────────────────────────────────────────────────

_CORR_GROUPS = {
    "GroupA_IndexETF":  {"ES", "SPY", "QQQ", "NQ", "IWM"},
    "GroupB_SemiETF":   {"SMH", "SOXX"},
    "GroupC_BigTech":   {"NVDA", "AAPL", "MSFT", "AMZN", "GOOG", "META", "AVGO", "TSM", "AMD", "TSLA"},
    "GroupD_Financial": {"JPM", "GS"},
}


def _assign_corr_group(code: str) -> str:
    for group, members in _CORR_GROUPS.items():
        if code in members:
            return group
    return "GroupZ_Other"


# ─── 优化报告解析 ──────────────────────────────────────────────────────────────

_OPT_PATTERN = re.compile(
    r'^\s*1\s+(\{[^}]+\})\s+([\d.]+)\s+([\d.]+)%\s+([\d.]+)\s+(-[\d.]+)%\s+([\d.]+)\s+([\d.]+)%'
)
_OPTION_DAYS_PATTERN = re.compile(r"'option_days':\s*(\d+)")


def _parse_rank1_metrics(path: Path) -> dict | None:
    if not path.exists():
        return None
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = _OPT_PATTERN.match(line)
            if m:
                days_m = _OPTION_DAYS_PATTERN.search(m.group(1))
                return {
                    "option_days":  int(days_m.group(1)) if days_m else 0,
                    "sharpe":       float(m.group(2)),
                    "win_rate":     float(m.group(3)) / 100,
                    "pnl_ratio":    float(m.group(4)),
                    "max_dd":       float(m.group(5)) / 100,
                    "recov_factor": float(m.group(6)),
                    "total_return": float(m.group(7)) / 100,
                }
    return None


# ─── 报告文件扫描 ──────────────────────────────────────────────────────────────

def _find_opt_files(output_dir: Path) -> list[tuple[str, str, Path]]:
    results = []
    pattern = re.compile(r'UTurnBottomShortPut_([A-Z0-9]+)_(M\d+|H\d+)_opt\.txt$')
    for p in sorted(output_dir.glob("*/optimizer/UTurnBottomShortPut_*_opt.txt")):
        m = pattern.search(p.name)
        if m:
            results.append((m.group(1), m.group(2), p))
    return results


_MAX_DD_FLOOR = -0.007  # 0.7%：回撤过小时按此下限重算收益风险比，防止小样本虚高


def _apply_max_dd_floor(df: pd.DataFrame) -> pd.DataFrame:
    """最大回撤绝对值 < 0.7% 时，按 0.7% 重新计算 recov_factor。"""
    df = df.copy()
    mask = df["max_dd"] > _MAX_DD_FLOOR
    df.loc[mask, "max_dd"] = _MAX_DD_FLOOR
    df.loc[mask, "recov_factor"] = df.loc[mask, "total_return"] / abs(_MAX_DD_FLOOR)
    return df


def _load_all_metrics(output_dir: Path) -> pd.DataFrame:
    rows = []
    for code, interval, path in _find_opt_files(output_dir):
        metrics = _parse_rank1_metrics(path)
        if metrics:
            rows.append({"code": code, "interval": interval, **metrics})
    return pd.DataFrame(rows)


# ─── 保证金估算 ────────────────────────────────────────────────────────────────

def _max_concurrent_positions(df: pd.DataFrame) -> int:
    df = df.copy()
    df["entry_dt"] = pd.to_datetime(df["entry_dt"])
    df["settlement_dt"] = pd.to_datetime(df["settlement_dt"])
    events = (
        [(t, 1) for t in df["entry_dt"]] +
        [(t, -1) for t in df["settlement_dt"]]
    )
    events.sort(key=lambda x: (x[0], x[1]))  # 同时刻结算(-1)先于开仓(+1)
    max_c, cur = 0, 0
    for _, delta in events:
        cur += delta
        max_c = max(max_c, cur)
    return max(max_c, 1)


def _estimate_margin(deallist_csv: Path, multiplier: float,
                     recent_n: int = 20) -> tuple[float, int, int]:
    """返回 (avg_margin_per_trade, max_concurrent_positions, total_trades)
    avg_margin 取最近 recent_n 笔的均值，反映当前价格水平。"""
    if not deallist_csv.exists():
        return 0.0, 1, 0
    df = pd.read_csv(deallist_csv)
    if df.empty or "k1" not in df.columns:
        return 0.0, 1, 0
    recent = df.tail(recent_n)
    avg_margin = (recent["k1"] - recent["k2"] - recent["net_credit"]).mean() * multiplier
    max_conc = _max_concurrent_positions(df)
    return avg_margin, max_conc, len(df)


def _strategy_direction(strategy: str) -> str:
    """根据策略类名推导方向：含 'Call' 为 SHORT，否则为 LONG。"""
    return "SHORT" if "Call" in strategy else "LONG"


def _deallist_csv(output_dir: Path, code: str, interval: str, strategy: str) -> Path:
    direction = _strategy_direction(strategy)
    return output_dir / code / "deallist" / f"{strategy}_{code}_{interval}_{direction}.csv"


def _add_margin_column(df: pd.DataFrame, output_dir: Path,
                       multipliers: dict[str, float],
                       simulate_max1: bool = False,
                       recent_n: int = 20) -> pd.DataFrame:
    margins, concurrents, trades = [], [], []
    for _, row in df.iterrows():
        code, interval = row["code"], row["interval"]
        strategy = row.get("strategy", "UTurnBottomShortPut")
        mult = multipliers.get(code, 100.0)
        csv = _deallist_csv(output_dir, code, interval, strategy)
        margin, conc, n = _estimate_margin(csv, mult, recent_n)
        if simulate_max1:
            conc = 1
        margins.append(margin)
        concurrents.append(conc)
        trades.append(n)
    df = df.copy()
    df["margin_per_trade"] = margins
    df["max_concurrent"] = concurrents
    df["total_trades"] = trades
    df["peak_margin"] = df["margin_per_trade"] * df["max_concurrent"]
    return df


# ─── 组合概览打印 ──────────────────────────────────────────────────────────────

def _print_portfolio(df: pd.DataFrame, capital: float) -> None:
    total_peak = df["peak_margin"].sum()
    sep = "═" * 90
    print(f"\n{sep}\n  投资组合概览\n{sep}")
    for _, r in df.iterrows():
        alloc_pct = r["peak_margin"] / capital * 100
        print(f"  {r['code']:6s} {r['interval']:4s} | 组别:{r['group']:20s} | "
              f"Sharpe:{r['sharpe']:.2f} WinRate:{r['win_rate']:.1%} MaxDD:{r['max_dd']:.2%} "
              f"RecovF:{r['recov_factor']:.1f} | "
              f"峰值保证金:${r['peak_margin']:,.0f} ({alloc_pct:.1f}%)")
    print(f"\n  总峰值保证金占用: ${total_peak:,.0f} / ${capital:,.0f} ({total_peak/capital:.1%})")
    print(f"  闲置缓冲资金:     ${capital - total_peak:,.0f}\n")


# ─── 以下为原有函数 ────────────────────────────────────────────────────────────

def _load_multipliers(settings_path: Path) -> dict[str, float]:
    with open(settings_path, encoding="utf-8") as f:
        cfg = yaml.safe_load(f)
    return {k: v.get("multiplier", 100.0)
            for k, v in cfg.get("codeIds", {}).items()}


def _load_portfolio_deallists(portfolio: pd.DataFrame, output_dir: Path,
                               multipliers: dict[str, float]) -> pd.DataFrame:
    frames = []
    for _, row in portfolio.iterrows():
        code, interval = row["code"], row["interval"]
        strategy = row.get("strategy", "UTurnBottomShortPut")
        mult = multipliers.get(code, 100.0)
        csv = _deallist_csv(output_dir, code, interval, strategy)
        if not csv.exists():
            continue
        df = pd.read_csv(csv, parse_dates=["entry_dt", "settlement_dt"])
        df["code"] = code
        df["dollar_pnl"] = df["final_pnl"] * mult
        df["margin"] = (df["k1"] - df["k2"]) * mult
        frames.append(df)
    return pd.concat(frames, ignore_index=True) if frames else pd.DataFrame()


def _build_equity_curve(deals: pd.DataFrame, capital: float) -> pd.Series:
    pnl_by_date = deals.groupby("settlement_dt")["dollar_pnl"].sum().sort_index()
    equity = capital + pnl_by_date.cumsum()
    start = pnl_by_date.index[0] - pd.Timedelta(days=1)
    return pd.concat([pd.Series([capital], index=[start]), equity])


def _build_margin_timeline(deals: pd.DataFrame) -> pd.Series:
    events = (
        [(r["entry_dt"], r["margin"]) for _, r in deals.iterrows()] +
        [(r["settlement_dt"], -r["margin"]) for _, r in deals.iterrows()]
    )
    events.sort(key=lambda x: x[0])
    cur, times, vals = 0.0, [], []
    for t, delta in events:
        cur += delta
        times.append(t)
        vals.append(cur)
    return pd.Series(vals, index=pd.DatetimeIndex(times))


def _calc_max_drawdown(equity: pd.Series) -> tuple[float, pd.Timestamp]:
    dd = (equity - equity.cummax()) / equity.cummax()
    idx = dd.idxmin()
    return dd.min(), idx


def _calc_portfolio_sharpe(deals: pd.DataFrame, capital: float) -> float:
    monthly = deals.set_index("settlement_dt")["dollar_pnl"].resample("ME").sum()
    returns = monthly / capital
    return float(returns.mean() / returns.std() * (12 ** 0.5)) if returns.std() > 0 else float("nan")


def _calc_portfolio_metrics(deals: pd.DataFrame, capital: float) -> dict:
    equity = _build_equity_curve(deals, capital)
    margin_ts = _build_margin_timeline(deals)
    max_dd, max_dd_date = _calc_max_drawdown(equity)
    total_pnl = deals["dollar_pnl"].sum()
    wins   = deals.loc[deals["dollar_pnl"] > 0, "dollar_pnl"]
    losses = deals.loc[deals["dollar_pnl"] < 0, "dollar_pnl"]
    avg_win  = wins.mean()  if not wins.empty  else 0.0
    avg_loss = abs(losses.mean()) if not losses.empty else 1.0
    profit_factor = avg_win / avg_loss if avg_loss > 0 else float("inf")
    recov_factor = (total_pnl / capital) / abs(max_dd) if max_dd != 0 else float("inf")
    max_margin_date = margin_ts.idxmax()
    return {
        "sharpe": _calc_portfolio_sharpe(deals, capital),
        "total_pnl": total_pnl,
        "total_return": total_pnl / capital,
        "max_dd": max_dd,
        "max_dd_date": max_dd_date,
        "profit_factor": profit_factor,
        "recov_factor": recov_factor,
        "max_margin": margin_ts.max(),
        "max_margin_date": max_margin_date,
        "equity": equity,
        "margin_ts": margin_ts,
    }


def _print_portfolio_metrics(metrics: dict, capital: float) -> None:
    sep = "═" * 90
    print(f"\n{sep}\n  组合整体绩效\n{sep}")
    print(f"  总盈亏:            ${metrics['total_pnl']:,.2f}  (收益率: {metrics['total_return']:.2%})")
    print(f"  月度 Sharpe:       {metrics['sharpe']:.2f}")
    print(f"  最大回撤:          {metrics['max_dd']:.2%}  (发生时间: {metrics['max_dd_date'].strftime('%Y-%m-%d %H:%M')})")
    print(f"  盈亏比(PF):        {metrics['profit_factor']:.2f}")
    print(f"  收益回撤比:        {metrics['recov_factor']:.2f}")
    print(f"  最大资金占用:      ${metrics['max_margin']:,.0f} ({metrics['max_margin']/capital:.1%})"
          f"  (发生时间: {metrics['max_margin_date'].strftime('%Y-%m-%d %H:%M')})")
    print()


def _print_open_positions(deals: pd.DataFrame, portfolio: pd.DataFrame) -> None:
    sep = "═" * 90
    print(f"\n{sep}\n  数据截止时持仓快照\n{sep}")
    for _, row in portfolio.iterrows():
        code, interval = row["code"], row["interval"]
        sub = deals[deals["code"] == code].copy()
        if sub.empty:
            print(f"  {code:6s} {interval:4s} | 无数据")
            continue
        open_pos = sub[sub["close_reason"] == "force_close"]
        data_end = sub["settlement_dt"].max()
        if open_pos.empty:
            print(f"  {code:6s} {interval:4s} | 截止 {data_end.strftime('%Y-%m-%d %H:%M')} | 无未平仓")
            continue
        print(f"  {code:6s} {interval:4s} | 截止 {data_end.strftime('%Y-%m-%d %H:%M')} | 持仓 {len(open_pos)} 笔:")
        for _, p in open_pos.iterrows():
            held = (p["settlement_dt"] - p["entry_dt"]).days
            print(f"    入场:{p['entry_dt'].strftime('%Y-%m-%d %H:%M')}  "
                  f"强平:{p['settlement_dt'].strftime('%Y-%m-%d %H:%M')}  "
                  f"行权价:{p['k1']:.1f}/{p['k2']:.1f}  "
                  f"权利金:${p['net_credit']:.2f}  已持有:{held}天")
    print()


def _plot_portfolio(metrics: dict, capital: float, output_dir: Path) -> None:
    equity = metrics["equity"]
    margin_ts = metrics["margin_ts"]
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8), sharex=False)
    fig.suptitle("Portfolio Equity Curve & Margin Usage", fontsize=13)

    ax1.plot(equity.index, equity.values, color="steelblue", linewidth=1.5)
    ax1.axhline(capital, color="gray", linestyle="--", linewidth=0.8, label=f"Initial ${capital:,.0f}")
    ax1.fill_between(equity.index, equity.values, capital,
                     where=(equity.values >= capital), alpha=0.15, color="green")
    ax1.fill_between(equity.index, equity.values, capital,
                     where=(equity.values < capital), alpha=0.15, color="red")
    dd_date = metrics["max_dd_date"]
    ax1.axvline(dd_date, color="red", linestyle=":", linewidth=1, label=f"MaxDD {metrics['max_dd']:.2%}")
    ax1.set_ylabel("Equity ($)")
    ax1.legend(fontsize=8)
    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m"))
    ax1.xaxis.set_major_locator(mdates.MonthLocator(interval=2))
    plt.setp(ax1.get_xticklabels(), rotation=30, ha="right", fontsize=7)

    ax2.fill_between(margin_ts.index, margin_ts.values, alpha=0.4, color="orange", step="post")
    ax2.plot(margin_ts.index, margin_ts.values, color="darkorange", linewidth=1, drawstyle="steps-post")
    ax2.axhline(metrics["max_margin"], color="red", linestyle="--", linewidth=0.8,
                label=f"Peak ${metrics['max_margin']:,.0f}")
    ax2.set_ylabel("Margin Used ($)")
    ax2.legend(fontsize=8)
    ax2.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m"))
    ax2.xaxis.set_major_locator(mdates.MonthLocator(interval=2))
    plt.setp(ax2.get_xticklabels(), rotation=30, ha="right", fontsize=7)

    plt.tight_layout()
    out = output_dir / "portfolio"
    out.mkdir(parents=True, exist_ok=True)
    fig.savefig(out / "equity_curve.png", dpi=150)
    print(f"  资金曲线已保存至 {out}/equity_curve.png")
    plt.show()
