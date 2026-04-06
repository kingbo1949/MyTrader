"""
portfolio_analyzer.py — 手工指定组合的绩效分析程序

读取 config/analyze.yaml 中的 portfolio 字段，
加载对应 deallist，输出绩效指标和资金曲线。

运行: cd layer_app/StrategyEngine
      PYTHONPATH=ice:src python3 scripts/portfolio_analyzer.py
"""
import sys
from pathlib import Path

import pandas as pd
import yaml

BASE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(BASE / "src"))

from portfolio_perf import (
    _assign_corr_group,
    _parse_rank1_metrics,
    _add_margin_column,
    _print_portfolio,
    _load_multipliers,
    _load_portfolio_deallists,
    _calc_portfolio_metrics,
    _print_portfolio_metrics,
    _print_open_positions,
    _plot_portfolio,
)


def _load_config() -> dict:
    path = BASE / "config" / "portfolio_analyzer.yaml"
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f).get("analyze", {})


def _build_portfolio_df(manual_portfolio: list[dict]) -> pd.DataFrame:
    return pd.DataFrame([
        {"code": item["code"], "interval": item["interval"],
         "strategy": item.get("strategy", "UTurnBottomShortPut")}
        for item in manual_portfolio
    ])


def _enrich_portfolio(portfolio: pd.DataFrame, output_dir: Path,
                      multipliers: dict) -> pd.DataFrame:
    rows = []
    for _, row in portfolio.iterrows():
        code, interval, strategy = row["code"], row["interval"], row["strategy"]
        opt_path = output_dir / code / "optimizer" / f"{strategy}_{code}_{interval}_opt.txt"
        m = _parse_rank1_metrics(opt_path) or {}
        rows.append({"code": code, "interval": interval, "strategy": strategy, **m})
    df = pd.DataFrame(rows)
    df = _add_margin_column(df, output_dir, multipliers)
    df["group"] = df["code"].map(_assign_corr_group)
    return df


def _run_analysis(portfolio: pd.DataFrame, capital: float,
                  output_dir: Path, settings_path: Path) -> None:
    multipliers = _load_multipliers(settings_path)
    enriched = _enrich_portfolio(portfolio, output_dir, multipliers)
    _print_portfolio(enriched, capital)
    deals = _load_portfolio_deallists(portfolio, output_dir, multipliers)
    if deals.empty:
        print("  ⚠ 未找到成交记录，请确认 portfolio_analyzer.yaml 中的 code/interval 与 output/ 目录匹配")
        return
    metrics = _calc_portfolio_metrics(deals, capital)
    _print_portfolio_metrics(metrics, capital)
    _print_open_positions(deals, portfolio)
    _plot_portfolio(metrics, capital, output_dir)


def main() -> None:
    cfg = _load_config()
    capital = cfg.get("initial_capital", 100_000)
    manual_portfolio = cfg.get("portfolio")

    if not manual_portfolio:
        print("  ⚠ analyze.yaml 中未配置 portfolio，请取消注释示例并填写品种列表")
        sys.exit(0)

    portfolio = _build_portfolio_df(manual_portfolio)
    print(f"手工组合：{len(portfolio)} 个品种")
    for _, r in portfolio.iterrows():
        print(f"  {r['code']:6s} {r['interval']:4s} {r['strategy']}")

    output_dir = BASE / "output"
    settings_path = BASE / "config" / "settings.yaml"
    _run_analysis(portfolio, capital, output_dir, settings_path)


if __name__ == "__main__":
    main()
