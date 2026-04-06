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
    pnl_by_dt: dict = {}
    for dt, pnl in _match_roundtrips(trade_df, multiplier):
        pnl_by_dt[dt] = pnl_by_dt.get(dt, 0.0) + pnl
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

def _match_roundtrips(trade_df: pd.DataFrame, multiplier: float) -> list[tuple]:
    """FIFO 配对开/平仓，返回 (close_datetime, pnl) 列表。
    支持同方向持仓叠加（如 Bull Put Spread 重叠时多个 SHORT 同时在途）。
    """
    from collections import deque
    pending: deque[tuple] = deque()  # (direction, price)
    results = []
    for _, row in trade_df.iterrows():
        if not pending or pending[0][0] == row.direction:
            pending.append((row.direction, row.price))
        else:
            open_dir, open_price = pending.popleft()
            diff = (row.price - open_price) if open_dir == Direction.LONG else (open_price - row.price)
            results.append((pd.Timestamp(row.datetime), diff * multiplier))
    return results


def _round_trip_pnls(trade_df: pd.DataFrame, multiplier: float) -> list[float]:
    return [pnl for _, pnl in _match_roundtrips(trade_df, multiplier)]


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
    if std == 0:
        return 0.0
    n = len(equity)
    span_years = (equity.index[-1] - equity.index[0]).total_seconds() / (365.25 * 24 * 3600)
    bars_per_year = n / span_years if span_years > 0 else 252
    return (ret.mean() / std) * np.sqrt(bars_per_year)


def _calc_summary(df: pd.DataFrame, initial_capital: float) -> dict[str, Any]:
    total_return = df["equity"].iloc[-1] / initial_capital - 1
    max_dd_dt = df["drawdown"].idxmin()
    max_dd_pct = abs(df["drawdown"].min())
    max_dd_amount = (df["max_equity"] - df["equity"])[max_dd_dt]
    return {
        "final_equity": df["equity"].iloc[-1],
        "total_return": total_return,
        "max_dd_pct": df["drawdown"].min(),
        "max_dd_amount": max_dd_amount,
        "max_dd_dt": max_dd_dt,
        "sharpe": _calc_sharpe(df["equity"]),
        "recovery_factor": total_return / max_dd_pct if max_dd_pct != 0 else 0.0,
    }


# ─── 报告输出 ────────────────────────────────────────────────────────────────

def _print_report(summary: dict, trades: dict, initial_capital: float) -> None:
    s, t = summary, trades
    print("\n" + "█" * 44)
    print(f"┃ {'QUANT ENHANCED REPORT':^40} ┃")
    print("█" * 44)
    print(f"┃ 初始资金:    ${initial_capital:,.2f}")
    print(f"┃ 最终净值:    ${s['final_equity']:,.2f} ({s['total_return']:+.2%})")
    print(f"┃ 最大回撤:    {s['max_dd_pct']:.2%} (-${s['max_dd_amount']:,.2f})  @ {s['max_dd_dt']}")
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
        self._trade_metrics = _calc_trade_metrics(_round_trip_pnls(trade_df, self.multiplier))
        self._summary = _calc_summary(bar_df, self.initial_capital)
        _print_report(self._summary, self._trade_metrics, self.initial_capital)
        return bar_df

    def plot_result(self, df: pd.DataFrame, title: str = "", params: dict = None,
                    save_path: str = None, show: bool = True) -> None:
        if df.empty:
            return
        plt.style.use("ggplot")
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)
        if title:
            fig.suptitle(title, fontsize=15, fontweight="bold")
        ax1.plot(df.index, df["equity"], color="#2c3e50", lw=2, label="Equity Curve")
        ax1.set_title("Strategic Performance: Equity Curve", fontsize=14, fontweight="bold")
        ax1.set_ylabel("Account Value (USD)")
        ax1.legend(loc="upper left")
        s, t = getattr(self, '_summary', None), getattr(self, '_trade_metrics', None)
        if s and t:
            metrics_text = (
                f"Return: {s['total_return']:+.2%}   Sharpe: {s['sharpe']:.2f}   "
                f"MaxDD: {s['max_dd_pct']:.2%}   RecovFactor: {s['recovery_factor']:.2f}\n"
                f"Trades: {t['total_trades']}   WinRate: {t['win_rate']:.2%}   "
                f"PnLRatio: {t['pnl_ratio']:.2f}   AvgPnL: ${t['avg_pnl']:,.2f}"
            )
            ax1.text(0.99, 0.97, metrics_text, transform=ax1.transAxes,
                     ha='right', va='top', fontsize=9,
                     bbox=dict(boxstyle='round,pad=0.4', facecolor='white', alpha=0.85, edgecolor='#cccccc'))
        ax2.fill_between(df.index, df["drawdown"] * 100, 0,
                         facecolor="#e74c3c", alpha=0.5, label="Drawdown %")
        ax2.set_title("Risk Profile: Drawdown", fontsize=14, fontweight="bold")
        ax2.set_ylabel("Percentage (%)")
        if params:
            items = [f"{k}={v}" for k, v in params.items()]
            lines = ["    ".join(items[i:i+4]) for i in range(0, len(items), 4)]
            fig.text(0.5, 0.01, "\n".join(lines),
                     ha='center', va='bottom', fontsize=7.5, family='monospace',
                     bbox=dict(boxstyle='round,pad=0.3', facecolor='#f5f5f5', alpha=0.8))
            plt.tight_layout(rect=[0, 0.06 + 0.025 * len(lines), 1, 1])
        else:
            plt.tight_layout()
        if save_path:
            fig.savefig(save_path, dpi=150, bbox_inches='tight')
        if show:
            plt.show()
        else:
            plt.close(fig)
