"""
compare.py — 多品种策略绩效横向对比
读取 config/compare.yaml，按策略列表依次回测，输出原始指标表和杠杆折算表。
运行: cd scripts && python3 compare.py
"""
import logging
import sys
from pathlib import Path

import pandas as pd

import IBTrader

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from qib_trader.api.converters import IceConverter  # noqa: F401（触发 Ice 初始化）
from qib_trader.core.broker import make_broker, get_broker, reset_broker
from qib_trader.core.engine import BacktestEngine
from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.strategy_pool import make_pool, get_pool, reset_pool
from qib_trader.utils.config_loader import get_codeId_config, load_yaml_config
from qib_trader.utils.factory_ice import FactoryIce
from qib_trader.utils.log_setup import setup_logging
from qib_trader.utils.stats import PerformanceAnalyzer

logger = logging.getLogger("Compare")


# ─── 配置加载 ─────────────────────────────────────────────────────────────────

def _load_compare_config() -> dict:
    cfg = load_yaml_config("config/compare.yaml")
    return cfg.get("compare", {})


# ─── 数据获取 ─────────────────────────────────────────────────────────────────

def _fetch_bars(s_cfg: dict) -> list:
    interval_str = s_cfg["interval"]
    time_type = getattr(IBTrader.ITimeType, interval_str)
    time_pair = FactoryIce.make_timepair(s_cfg["start_time"], s_cfg["end_time"])
    df = FactoryIce.get_enriched_klines_loop(s_cfg["code_id"], time_type, time_pair)
    if df.empty:
        logger.warning(f"无数据: {s_cfg['code_id']} {interval_str}")
        return []
    return IceConverter.enriched_df_to_bars(df, s_cfg["code_id"], interval_str)


# ─── 单策略回测 ───────────────────────────────────────────────────────────────

def _collect_metrics(s_cfg: dict, strategy, bars: list, initial_capital: float) -> dict | None:
    """提取绩效指标并计算杠杆倍数。"""
    trades = [t for t in get_broker().get_trades() if t.strategy_id == strategy.strategy_id]
    multiplier = get_codeId_config(s_cfg["code_id"]).get("multiplier", 1.0)
    analyzer = PerformanceAnalyzer(trades, bars, initial_capital=initial_capital,
                                   contract_multiplier=multiplier, options_mode=strategy.options_mode)
    df = analyzer.calculate_performance()
    if df.empty:
        return None
    avg_price = sum(b.close_price for b in bars) / len(bars)
    leverage = (avg_price * multiplier) / initial_capital
    return {
        "code_id": s_cfg["code_id"],
        "interval": s_cfg["interval"],
        "leverage": leverage,
        **analyzer._summary,
        **analyzer._trade_metrics,
    }


def _run_single(s_cfg: dict, initial_capital: float) -> dict | None:
    """回测单个策略，返回指标字典，失败返回 None。"""
    bars = _fetch_bars(s_cfg)
    if not bars:
        return None
    reset_pool()
    reset_broker()
    full_cfg = {**s_cfg, "params": {**s_cfg.get("params", {}), "initial_capital": initial_capital}}
    strategy = FactoryStrategy.create_strategy(full_cfg)
    get_pool().add_strategy(strategy)
    BacktestEngine().run_backtest(bars)
    return _collect_metrics(s_cfg, strategy, bars, initial_capital)


# ─── 表格构建 ─────────────────────────────────────────────────────────────────

def _build_raw_table(results: list[dict]) -> pd.DataFrame:
    rows = []
    for r in results:
        rows.append({
            "品种": r["code_id"],
            "周期": r["interval"],
            "杠杆": f"{r['leverage']:.2f}x",
            "Return": f"{r['total_return']:+.2%}",
            "Sharpe": f"{r['sharpe']:.2f}",
            "MaxDD": f"{r['max_dd_pct']:.2%}",
            "RecovFactor": f"{r['recovery_factor']:.2f}",
            "Trades": r["total_trades"],
            "WinRate": f"{r['win_rate']:.2%}",
            "PnLRatio": f"{r['pnl_ratio']:.2f}",
            "AvgPnL": f"${r['avg_pnl']:,.0f}",
        })
    return pd.DataFrame(rows)


def _normalize_leverage(results: list[dict]) -> list[dict]:
    """将所有品种的 Return/MaxDD 折算到最低杠杆水平，Sharpe 保持不变。"""
    min_lev = min(r["leverage"] for r in results)
    adjusted = []
    for r in results:
        factor = min_lev / r["leverage"]
        adj_return = r["total_return"] * factor
        adj_dd = r["max_dd_pct"] * factor
        adj_recov = adj_return / abs(adj_dd) if adj_dd != 0 else 0.0
        adjusted.append({**r, "total_return": adj_return,
                         "max_dd_pct": adj_dd, "recovery_factor": adj_recov})
    return adjusted


def _build_adjusted_table(results: list[dict], ref_lev: float) -> pd.DataFrame:
    rows = []
    for r in results:
        rows.append({
            "品种": r["code_id"],
            "周期": r["interval"],
            f"Return(折至{ref_lev:.2f}x)": f"{r['total_return']:+.2%}",
            "Sharpe(不变)": f"{r['sharpe']:.2f}",
            f"MaxDD(折至{ref_lev:.2f}x)": f"{r['max_dd_pct']:.2%}",
            "RecovFactor": f"{r['recovery_factor']:.2f}",
        })
    return pd.DataFrame(rows)


# ─── 输出 ─────────────────────────────────────────────────────────────────────

def _print_table(title: str, df: pd.DataFrame) -> None:
    sep = "═" * max(60, len(title) + 4)
    print(f"\n{sep}")
    print(f"  {title}")
    print(sep)
    print(df.to_string(index=False))
    print()


def _save_output(raw_df: pd.DataFrame, adj_df: pd.DataFrame) -> None:
    out = Path(__file__).resolve().parents[1] / "output" / "compare"
    out.mkdir(parents=True, exist_ok=True)
    raw_df.to_csv(out / "raw_metrics.csv", index=False)
    adj_df.to_csv(out / "leverage_adjusted.csv", index=False)
    logger.info(f"对比结果已保存至 {out}/")


# ─── 主入口 ───────────────────────────────────────────────────────────────────

def main() -> None:
    setup_logging()
    make_pool()
    make_broker()
    try:
        cfg = _load_compare_config()
        initial_capital = cfg.get("initial_capital", 100_000.0)
        strategies = cfg.get("strategies", [])
        if not strategies:
            logger.error("compare.yaml 中无策略配置")
            return

        results = []
        for s_cfg in strategies:
            logger.info(f"回测: {s_cfg['code_id']} {s_cfg['interval']}")
            r = _run_single(s_cfg, initial_capital)
            if r:
                results.append(r)
            else:
                logger.warning(f"跳过 {s_cfg['code_id']} {s_cfg['interval']}（无结果）")

        if len(results) < 2:
            logger.error("至少需要 2 个有效结果才能对比")
            return

        raw_df = _build_raw_table(results)
        _print_table("原始绩效对比（未调杠杆）", raw_df)

        min_lev = min(r["leverage"] for r in results)
        adj_results = _normalize_leverage(results)
        adj_df = _build_adjusted_table(adj_results, min_lev)
        _print_table(f"杠杆折算对比（统一折至最低杠杆 {min_lev:.2f}x）", adj_df)

        _save_output(raw_df, adj_df)

    except Exception as e:
        logger.error(f"运行失败: {e}", exc_info=True)
    finally:
        FactoryIce.shutdown()
        logger.info("ICE 已关闭")


if __name__ == "__main__":
    main()
