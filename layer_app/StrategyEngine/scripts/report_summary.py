"""
report_summary.py — 全量优化报告汇总表

扫描 output/ 下所有品种×周期的优化报告第一名行，
汇总为按 score 降序排列的表格，供快速横向对比。

运行: cd layer_app/StrategyEngine
      PYTHONPATH=ice:src python3 scripts/report_summary.py
"""
import sys
from pathlib import Path

import yaml

BASE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(BASE / "src"))

from portfolio_perf import (
    _load_all_metrics,
    _apply_max_dd_floor,
    _load_multipliers,
    _add_margin_column,
)


def _load_config() -> dict:
    path = BASE / "config" / "portfolio_selector.yaml"
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f).get("portfolio", {})


def _calc_scores(df, weights: dict):
    df = df.copy()
    w_s = weights.get("sharpe", 1.0)
    w_r = weights.get("recov_factor", 1.0)
    w_t = weights.get("total_return", 10.0)
    df["score"] = w_s * df["sharpe"] + w_r * df["recov_factor"] + w_t * df["total_return"]
    return df.sort_values("score", ascending=False).reset_index(drop=True)


def _print_summary(df) -> None:
    disp = df[["code", "interval", "sharpe", "win_rate", "pnl_ratio", "max_dd",
               "recov_factor", "total_return", "total_trades", "max_concurrent",
               "option_days", "score"]].copy()
    disp["win_rate"]     = disp["win_rate"].map("{:.1%}".format)
    disp["max_dd"]       = disp["max_dd"].map("{:.2%}".format)
    disp["total_return"] = disp["total_return"].map("{:.1%}".format)
    disp["sharpe"]       = disp["sharpe"].map("{:.2f}".format)
    disp["recov_factor"] = disp["recov_factor"].map("{:.1f}".format)
    disp["option_days"]  = disp["option_days"].map("{:.0f}".format)
    disp["score"]        = disp["score"].map("{:.1f}".format)
    sep = "═" * 110
    print(f"\n{sep}\n  全量优化报告汇总（所有品种×周期，按 score 降序）\n{sep}")
    print(disp.to_string(index=False))
    print()


def _save_summary(df, output_dir: Path) -> None:
    out = output_dir / "summary"
    out.mkdir(parents=True, exist_ok=True)
    csv_path = out / "all_scores.csv"
    float_cols = df.select_dtypes(include="float").columns
    df.round({col: 4 for col in float_cols}).to_csv(csv_path, index=False)
    print(f"  结果已保存至 {csv_path}")


def main() -> None:
    cfg = _load_config()
    weights      = cfg.get("score_weights", {})
    output_dir   = BASE / "output"
    settings_path = BASE / "config" / "settings.yaml"

    print("正在读取优化报告...")
    df = _load_all_metrics(output_dir)
    df = _apply_max_dd_floor(df)
    print(f"  读取到 {len(df)} 条记录（{df['code'].nunique()} 个品种，"
          f"{df['interval'].nunique()} 种周期）")

    recent_n = cfg.get("margin_recent_n", 20)
    multipliers = _load_multipliers(settings_path)
    df = _add_margin_column(df, output_dir, multipliers, recent_n=recent_n)
    df = _calc_scores(df, weights)
    _print_summary(df)
    _save_summary(df, output_dir)


if __name__ == "__main__":
    main()
