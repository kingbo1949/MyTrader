import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from dataclasses import asdict
from typing import Any

from qib_trader.core.models import BarDatas, TradeDatas, Direction


# ─── 数据准备 ───────────────────────────────────────────────────────────────

def _build_bar_df(bars: BarDatas) -> pd.DataFrame:
    df = pd.DataFrame([asdict(b) for b in bars])
    df["datetime"] = pd.to_datetime(df["datetime"])
    df.set_index("datetime", inplace=True)
    return df[["open_price", "high_price", "low_price", "close_price", "volume"]]


def _build_trade_df(trades: TradeDatas) -> pd.DataFrame:
    df = pd.DataFrame([asdict(t) for t in trades])
    df["datetime"] = pd.to_datetime(df["datetime"])
    df["pos_change"] = df.apply(_pos_change, axis=1)
    return df


def _pos_change(row) -> float:
    return row.volume if row.direction == Direction.LONG else -row.volume


# ─── 净值曲线 ────────────────────────────────────────────────────────────────

def _merge_pos_into_bars(bar_df: pd.DataFrame, trade_df: pd.DataFrame) -> pd.DataFrame:
    bar_df = bar_df.copy()
    bar_df["pos_change"] = (
        trade_df.groupby("datetime")["pos_change"].sum()
        .reindex(bar_df.index)
        .fillna(0)
    )
    bar_df["position"] = bar_df["pos_change"].cumsum()
    return bar_df


def _realized_pnl_by_bar(trade_df: pd.DataFrame, multiplier: float, bar_index) -> pd.Series:
    """逐笔配对计算实现损益，按结算时间戳映射到 bar 索引。"""
    pnl_by_dt = {}
    pos, entry_price = 0, 0.0
    for _, row in trade_df.iterrows():
        if pos == 0:
            entry_price = row.price
            pos = 1 if row.direction == Direction.LONG else -1
        else:
            diff = (row.price - entry_price) if pos > 0 else (entry_price - row.price)
            dt = pd.Timestamp(row.datetime)
            pnl_by_dt[dt] = pnl_by_dt.get(dt, 0.0) + diff * multiplier
            pos = 0
    return pd.Series(pnl_by_dt).reindex(bar_index).fillna(0.0)


def _calc_equity(bar_df: pd.DataFrame, trade_df: pd.DataFrame,
                 initial_capital: float, multiplier: float,
                 options_mode: bool = False) -> pd.DataFrame:
    bar_df = bar_df.copy()
    if options_mode and not trade_df.empty:
        # 期权模式：用实现损益（FIFO配对）构建资金曲线，忽略持仓盯市
        bar_df["pnl"] = _realized_pnl_by_bar(trade_df, multiplier, bar_df.index)
    else:
        # 期货/股票模式：逐 bar 盯市
        bar_df["pnl"] = (
            bar_df["position"].shift(1).fillna(0)
            * bar_df["close_price"].diff().fillna(0)
            * multiplier
        )
    bar_df["equity"] = initial_capital + bar_df["pnl"].cumsum()
    bar_df["max_equity"] = bar_df["equity"].expanding().max()
    bar_df["drawdown"] = (bar_df["equity"] - bar_df["max_equity"]) / bar_df["max_equity"]
    return bar_df


# ─── 逐笔统计 ────────────────────────────────────────────────────────────────

def _round_trip_pnls(trade_df: pd.DataFrame, multiplier: float) -> list[float]:
    pnls, pos, entry_price = [], 0, 0.0
    for _, row in trade_df.iterrows():
        if pos == 0:
            entry_price, pos = row.price, (1 if row.direction == Direction.LONG else -1)
        else:
            diff = (row.price - entry_price) if pos > 0 else (entry_price - row.price)
            pnls.append(diff * multiplier)
            pos = 0
    return pnls


def _calc_trade_metrics(pnls: list[float]) -> dict[str, Any]:
    if not pnls:
        return {"total_trades": 0, "win_rate": 0.0, "pnl_ratio": 0.0, "avg_pnl": 0.0,
                "win_count": 0, "loss_count": 0}
    s = pd.Series(pnls)
    wins, losses = s[s > 0], s[s < 0]
    avg_win = wins.mean() if not wins.empty else 0.0
    avg_loss = abs(losses.mean()) if not losses.empty else 1.0
    return {
        "total_trades": len(pnls),
        "win_count": len(wins),
        "loss_count": len(losses),
        "win_rate": len(wins) / len(pnls),
        "pnl_ratio": avg_win / avg_loss,
        "avg_pnl": s.mean(),
    }


# ─── 汇总指标 ────────────────────────────────────────────────────────────────

def _calc_sharpe(equity: pd.Series) -> float:
    ret = equity.pct_change().fillna(0)
    std = ret.std()
    return (ret.mean() / std) * np.sqrt(252) if std != 0 else 0.0


def _calc_summary(df: pd.DataFrame, initial_capital: float) -> dict[str, Any]:
    total_pnl = df["equity"].iloc[-1] - initial_capital
    max_dd_amount = (df["max_equity"] - df["equity"]).max()
    return {
        "final_equity": df["equity"].iloc[-1],
        "total_return": df["equity"].iloc[-1] / initial_capital - 1,
        "max_dd_pct": df["drawdown"].min(),
        "max_dd_amount": max_dd_amount,
        "sharpe": _calc_sharpe(df["equity"]),
        "recovery_factor": total_pnl / max_dd_amount if max_dd_amount != 0 else 0.0,
    }


# ─── 报告输出 ────────────────────────────────────────────────────────────────

def _print_report(summary: dict, trades: dict, initial_capital: float) -> None:
    s, t = summary, trades
    print("\n" + "█" * 44)
    print(f"┃ {'QUANT ENHANCED REPORT':^40} ┃")
    print("█" * 44)
    print(f"┃ 初始资金:    ${initial_capital:,.2f}")
    print(f"┃ 最终净值:    ${s['final_equity']:,.2f} ({s['total_return']:+.2%})")
    print(f"┃ 最大回撤:    {s['max_dd_pct']:.2%} (-${s['max_dd_amount']:,.2f})")
    print(f"┃ 夏普比率:    {s['sharpe']:.2f}")
    print(f"┃ 收益风险比:   {s['recovery_factor']:.2f}")
    print("┣" + "━" * 42 + "┫")
    print(f"┃ 总平仓次数:   {t['total_trades']}")
    print(f"┃ 胜率:        {t['win_rate']:.2%}")
    print(f"┃ 盈亏比:      {t['pnl_ratio']:.2f}")
    print(f"┃ 单笔平均盈亏:  ${t['avg_pnl']:,.2f}")
    print("█" * 44 + "\n")


# ─── 主类 ────────────────────────────────────────────────────────────────────

class PerformanceAnalyzer:

    def __init__(self, trades: TradeDatas, bars: BarDatas,
                 initial_capital: float = 100_000.0,
                 contract_multiplier: float = 1.0,
                 options_mode: bool = False):
        self.trades = trades
        self.bars = bars
        self.initial_capital = initial_capital
        self.multiplier = contract_multiplier
        self.options_mode = options_mode

    def calculate_performance(self) -> pd.DataFrame:
        if not self.trades:
            print("警告: 无成交记录，无法计算绩效报告。")
            return pd.DataFrame()
        bar_df = _build_bar_df(self.bars)
        trade_df = _build_trade_df(self.trades)
        bar_df = _merge_pos_into_bars(bar_df, trade_df)
        bar_df = _calc_equity(bar_df, trade_df, self.initial_capital, self.multiplier, self.options_mode)
        trade_metrics = _calc_trade_metrics(_round_trip_pnls(trade_df, self.multiplier))
        summary = _calc_summary(bar_df, self.initial_capital)
        _print_report(summary, trade_metrics, self.initial_capital)
        return bar_df

    def plot_result(self, df: pd.DataFrame) -> None:
        if df.empty:
            return
        plt.style.use("ggplot")
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)
        ax1.plot(df.index, df["equity"], color="#2c3e50", lw=2, label="Equity Curve")
        ax1.set_title("Strategic Performance: Equity Curve", fontsize=14, fontweight="bold")
        ax1.set_ylabel("Account Value (USD)")
        ax1.legend(loc="upper left")
        ax2.fill_between(df.index, df["drawdown"] * 100, 0,
                         facecolor="#e74c3c", alpha=0.5, label="Drawdown %")
        ax2.set_title("Risk Profile: Drawdown", fontsize=14, fontweight="bold")
        ax2.set_ylabel("Percentage (%)")
        plt.tight_layout()
        plt.show()
