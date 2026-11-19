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


def _lifecycle_pnl_by_bar(closed_positions: list, multiplier: float, bar_index) -> pd.Series:
    """从 _closed_positions 按 settlement_dt 计算每 bar 实现损益，无 FIFO 错配问题。"""
    pnl_by_dt: dict = {}
    for p in closed_positions:
        dt = pd.Timestamp(p['settlement_dt'])
        pnl_by_dt[dt] = pnl_by_dt.get(dt, 0.0) + p['final_pnl'] * multiplier
    return pd.Series(pnl_by_dt).reindex(bar_index).fillna(0.0)


_AUXILIARY_REASONS = frozenset({
    'roll_expiry',             # call spread 中间滚仓
    'put_roll_expiry',         # put spread 中间滚仓
    'put_assignment',          # put spread 接货（新单元，非完整平仓）
    'put_assignment_rejected', # put spread 接货失败 MTM 平仓
    'put_cancelled_call',      # call 被 call away 时同步 MTM 平仓 put spread
    'put_cancelled_force',     # force_close 时同步 MTM 平仓 put spread
})


def _lifecycle_trade_metrics(closed_positions: list, multiplier: float) -> dict:
    """用 _closed_positions.final_pnl 计算胜率等指标，避免多持仓 FIFO 错配。
    中间滚仓和辅助事件记录不代表完整交易，排除后保持交易计数与 max_concurrent 口径一致。
    """
    pnls = [p['final_pnl'] * multiplier for p in closed_positions
            if p.get('close_reason') not in _AUXILIARY_REASONS]
    return _calc_trade_metrics(pnls)


def _build_virtual_nav(closed_positions: list, bar_index) -> pd.Series | None:
    """构建虚拟净值曲线 V（期权价差策略专用）。

    V_0 = 1.0
    V_t = V_{t-1} + final_pnl / notional   （notional = k1 - k2，即最大风险敞口）

    final_pnl 与 k1-k2 单位相同（价格点数），比值为无量纲数，
    与初始资金、合约乘数、持仓手数均无关。
    需要 closed_positions 中含有 k1、k2 字段（两个策略均已存储）。
    """
    if not closed_positions:
        return None
    if not all('k1' in p and 'k2' in p for p in closed_positions):
        return None   # 旧策略或期货策略，无 k1/k2，降级处理
    delta_v: dict = {}
    for p in closed_positions:
        phase = p.get('phase', 'spread')
        if phase == 'covered_call':
            # CC 阶段：持有正股，名义敞口 = 股票买入价 S0（= K1）
            # final_pnl 可能远超 k1-k2（股票大幅上涨），必须用 S0 归一化
            notional = p.get('S0', p['k1'])
        else:
            # 价差阶段：名义敞口 = 最大风险（差价宽度）
            notional = p['k1'] - p['k2']
        if notional <= 0:
            continue   # 跳过非法记录
        dt = pd.Timestamp(p['settlement_dt'])
        delta_v[dt] = delta_v.get(dt, 0.0) + p['final_pnl'] / notional
    if not delta_v:
        return None
    v = (pd.Series(delta_v)
         .reindex(bar_index)
         .fillna(0.0)
         .cumsum()
         .add(1.0)
         .ffill())
    return v


def _calc_virtual_metrics(v: pd.Series) -> dict:
    """从虚拟净值序列计算资金无关的绩效指标。"""
    v_max        = v.expanding().max()
    v_dd         = (v - v_max) / v_max
    total_return = float(v.iloc[-1] - 1.0)
    max_dd       = float(abs(v_dd.min()))
    rf           = total_return / max_dd if max_dd != 0 else 0.0
    return {'virtual_total_return': total_return, 'virtual_max_dd': max_dd, 'virtual_rf': rf}


def _calc_equity(bar_df: pd.DataFrame, trade_df: pd.DataFrame,
                 initial_capital: float, multiplier: float,
                 options_mode: bool = False,
                 mtm_pnl_series: dict | None = None,
                 closed_positions: list | None = None) -> pd.DataFrame:
    bar_df = bar_df.copy()
    if options_mode and not trade_df.empty:
        # 期权模式：优先用 lifecycle 记录（无 FIFO 错配），否则降级到 FIFO 配对
        if closed_positions:
            bar_df["pnl"] = _lifecycle_pnl_by_bar(closed_positions, multiplier, bar_df.index)
        else:
            bar_df["pnl"] = _realized_pnl_by_bar(trade_df, multiplier, bar_df.index)
    else:
        # 期货/股票模式：逐 bar 盯市
        bar_df["pnl"] = (
            bar_df["position"].shift(1).fillna(0)
            * bar_df["close_price"].diff().fillna(0)
            * multiplier
        )
    realized = initial_capital + bar_df["pnl"].cumsum()

    if options_mode and mtm_pnl_series:
        # 叠加当前未平仓持仓的盯市浮动盈亏，得到真实净值曲线
        mtm = (pd.Series(mtm_pnl_series)
               .reindex(bar_df.index)
               .ffill()
               .fillna(0.0)
               * multiplier)
        bar_df["equity"] = realized + mtm
    else:
        bar_df["equity"] = realized

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
                "win_count": 0, "loss_count": 0, "profit_factor": 0.0}
    s = pd.Series(pnls)
    wins, losses = s[s > 0], s[s < 0]
    avg_win   = wins.mean()        if not wins.empty   else 0.0
    avg_loss  = abs(losses.mean()) if not losses.empty else 1.0
    total_win  = wins.sum()
    total_loss = abs(losses.sum())
    profit_factor = total_win / total_loss if total_loss > 0 else float('inf')
    return {
        "total_trades":  len(pnls),
        "win_count":     len(wins),
        "loss_count":    len(losses),
        "win_rate":      len(wins) / len(pnls),
        "pnl_ratio":     avg_win / avg_loss,
        "avg_pnl":       s.mean(),
        "profit_factor": profit_factor,
    }


# ─── 汇总指标 ────────────────────────────────────────────────────────────────

def _calc_alpha(df: pd.DataFrame, v: pd.Series | None = None) -> float:
    """Alpha 系数（%/年）。

    使用月末权益百分比收益率做 OLS 回归，量级正常（年化 0-30%）。
    参数 v 保留但不再用于 alpha，权益月度法已足够准确且量级直观。

    r_s = equity 月末收益率（%），r_m = close_price 月末收益率（%）
    β = Cov(r_s, r_m) / Var(r_m)
    α_annual = (mean(r_s) - β × mean(r_m)) × 12   (%/年)

    降级（月度样本 < 3）：使用 equity.diff() 美元差分法，α 单位为 $/年。
    """
    eq_m = df["equity"].resample('ME').last()
    c_m  = df["close_price"].resample('ME').last()
    r_s  = eq_m.pct_change().dropna()
    r_m  = c_m.pct_change().reindex(r_s.index).dropna()
    r_s, r_m = r_s.align(r_m, join='inner')
    if len(r_s) >= 3:
        var_m = r_m.var()
        if var_m == 0:
            return 0.0
        beta            = r_s.cov(r_m) / var_m
        alpha_per_month = r_s.mean() - beta * r_m.mean()
        return alpha_per_month * 12
    # ── 降级：美元差分法 ─────────────────────────────────────────────────────
    r_s   = df["equity"].diff().fillna(0)
    r_m   = df["close_price"].diff().fillna(0)
    var_m = r_m.var()
    if var_m == 0:
        return 0.0
    beta          = r_s.cov(r_m) / var_m
    alpha_per_bar = r_s.mean() - beta * r_m.mean()
    span_years    = (df.index[-1] - df.index[0]).total_seconds() / (365.25 * 24 * 3600)
    bars_per_year = len(df) / span_years if span_years > 0 else 252
    return alpha_per_bar * bars_per_year


def _calc_sharpe(equity: pd.Series) -> float:
    # 用绝对盈亏（美元差分）而非百分比收益率，使夏普与初始资金规模无关
    pnl = equity.diff().fillna(0)
    std = pnl.std()
    if std == 0:
        return 0.0
    n = len(equity)
    span_years = (equity.index[-1] - equity.index[0]).total_seconds() / (365.25 * 24 * 3600)
    bars_per_year = n / span_years if span_years > 0 else 252
    return (pnl.mean() / std) * np.sqrt(bars_per_year)


def _calc_smoothness(equity: pd.Series) -> dict[str, float]:
    """净值曲线"平滑稳定向上"的程度：把 equity 对 bar 序号做线性回归。
    用等权 bar 序号（而非真实时间间隔）做自变量——回测的 bar 本身就是逐根等间隔
    产生的，用序号能避免非交易时段的日历跨度扭曲拟合。

    smooth_r2：拟合优度，越接近1，净值曲线越接近一条匀速上升的直线。
    smooth_k_ratio：回归斜率的 t 统计量（斜率 / 斜率标准误），K-ratio 的简化版本，
    CTA/趋势策略常用来衡量"涨得稳不稳"——斜率为正且这个值越大，说明上涨趋势
    越显著、曲线偏离这条趋势线的抖动越小；只看总收益/总回撤看不出这一点
    （比如少数几次暴涨夹杂持续小亏，总回报可能不低，但这个值会很低）。
    """
    n = len(equity)
    if n < 3:
        return {"smooth_r2": 0.0, "smooth_k_ratio": 0.0}
    x = np.arange(n, dtype=float)
    y = equity.to_numpy(dtype=float)
    x_mean, y_mean = x.mean(), y.mean()
    sxx = np.sum((x - x_mean) ** 2)
    if sxx == 0:
        return {"smooth_r2": 0.0, "smooth_k_ratio": 0.0}
    slope = np.sum((x - x_mean) * (y - y_mean)) / sxx
    y_pred = (y_mean - slope * x_mean) + slope * x
    ss_res = np.sum((y - y_pred) ** 2)
    ss_tot = np.sum((y - y_mean) ** 2)
    r2 = 1 - ss_res / ss_tot if ss_tot > 0 else 0.0
    dof = n - 2
    if dof <= 0:
        return {"smooth_r2": float(r2), "smooth_k_ratio": 0.0}
    slope_se = np.sqrt(ss_res / dof) / np.sqrt(sxx)
    if slope_se == 0:
        # 残差为0（完美直线）：斜率为0是真的没有趋势，斜率非0则是最平滑的情况
        k_ratio = 0.0 if slope == 0 else float('inf') if slope > 0 else float('-inf')
    else:
        k_ratio = slope / slope_se
    return {"smooth_r2": float(r2), "smooth_k_ratio": float(k_ratio)}


def _calc_cagr(df: pd.DataFrame, initial_capital: float) -> float:
    """年化复合收益率，用真实经过的日历年数（而非 bar 计数）——跟 _calc_sharpe
    的年化口径一致，避免intraday bar频率不同导致年数被算错。"""
    final_equity = df["equity"].iloc[-1]
    span_years   = (df.index[-1] - df.index[0]).total_seconds() / (365.25 * 24 * 3600)
    if span_years <= 0 or final_equity <= 0 or initial_capital <= 0:
        return 0.0
    return (final_equity / initial_capital) ** (1.0 / span_years) - 1.0


def _calc_summary(df: pd.DataFrame, initial_capital: float,
                  closed_positions: list | None = None) -> dict[str, Any]:
    total_return  = df["equity"].iloc[-1] / initial_capital - 1
    max_dd_dt     = df["drawdown"].idxmin()
    max_dd_pct    = abs(df["drawdown"].min())
    max_dd_amount = (df["max_equity"] - df["equity"])[max_dd_dt]
    # ── 利润安全垫倍数：美元法（单策略自身的现金生存性参考，不用于跨策略排名）──
    # 总盈亏($) / 最大峰谷回撤($)，两者均为绝对美元，initial_capital 完全约去。
    # 物理意义：当前累积的净利润，能扛住多少次同等规模的历史最惨美元回撤。
    # 注：虚拟净值法（k1-k2 归一化）对期权价差策略会因尺度失配（160x）导致该值虚高。
    # 不适合跨策略/跨参数排名——见 recovery_factor_pct / calmar_ratio。
    total_pnl     = df["equity"].iloc[-1] - initial_capital
    max_dollar_dd = (df["max_equity"] - df["equity"]).max()
    profit_cushion_multiple = total_pnl / max_dollar_dd if max_dollar_dd > 0 else 0.0
    # ── 收益风险比 / Calmar：百分比法（资金规模无关，跨策略/跨参数排名用）─────
    # recovery_factor_pct：总收益% / 最大回撤%，不年化，仅在比较对象回测区间
    #   长度相同时（如 optimizer.py 网格搜索）才和 calmar_ratio 排序结果一致。
    # calmar_ratio：年化收益率(CAGR) / 最大回撤%，跨不同时间跨度比较时必须用它
    #   （否则区间越长，总收益% 越容易滚大，比较会不公平）。
    recovery_factor_pct = total_return / max_dd_pct if max_dd_pct > 0 else 0.0
    cagr          = _calc_cagr(df, initial_capital)
    calmar_ratio  = cagr / max_dd_pct if max_dd_pct > 0 else 0.0
    # ── 虚拟净值序列（仅用于 Alpha 计算）────────────────────────────────────
    v = _build_virtual_nav(closed_positions or [], df.index)
    smoothness = _calc_smoothness(df["equity"])
    return {
        "final_equity":  df["equity"].iloc[-1],
        "total_return":  total_return,
        "max_dd_pct":    df["drawdown"].min(),
        "max_dd_amount": max_dd_amount,
        "max_dd_dt":     max_dd_dt,
        "sharpe":          _calc_sharpe(df["equity"]),
        "alpha":           _calc_alpha(df, v),   # 传入已构建的虚拟净值序列
        "cagr":                    cagr,
        "profit_cushion_multiple": profit_cushion_multiple,
        "recovery_factor_pct":     recovery_factor_pct,
        "calmar_ratio":            calmar_ratio,
        "smooth_r2":       smoothness["smooth_r2"],
        "smooth_k_ratio":  smoothness["smooth_k_ratio"],
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
    print(f"┃ Calmar比率(年化):    {s['calmar_ratio']:.2f}   收益风险比(%):  {s['recovery_factor_pct']:.2f}")
    print(f"┃ 利润安全垫倍数($):    {s['profit_cushion_multiple']:.2f}")
    print(f"┃ 平滑度R²:    {s['smooth_r2']:.2f}   K-ratio: {s['smooth_k_ratio']:.2f}")
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
                 options_mode: bool = False,
                 mtm_pnl_series: dict | None = None,
                 closed_positions: list | None = None):
        self.trades = trades
        self.bars = bars
        self.initial_capital = initial_capital
        self.multiplier = contract_multiplier
        self.options_mode = options_mode
        self.mtm_pnl_series = mtm_pnl_series or {}
        self.closed_positions = closed_positions or []

    def calculate_performance(self) -> pd.DataFrame:
        if not self.trades:
            print("警告: 无成交记录，无法计算绩效报告。")
            return pd.DataFrame()
        bar_df = _build_bar_df(self.bars)
        trade_df = _build_trade_df(self.trades)
        bar_df = _merge_pos_into_bars(bar_df, trade_df)
        bar_df = _calc_equity(bar_df, trade_df, self.initial_capital, self.multiplier,
                              self.options_mode, self.mtm_pnl_series, self.closed_positions)
        if self.closed_positions:
            self._trade_metrics = _lifecycle_trade_metrics(self.closed_positions, self.multiplier)
        else:
            self._trade_metrics = _calc_trade_metrics(_round_trip_pnls(trade_df, self.multiplier))
        self._summary = _calc_summary(bar_df, self.initial_capital, self.closed_positions)
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
        ax1.plot(df.index, df["equity"], color="#2c3e50", lw=2)
        ax1.set_title("Strategic Performance: Equity Curve", fontsize=14, fontweight="bold")
        ax1.set_ylabel("Account Value (USD)")
        s, t = getattr(self, '_summary', None), getattr(self, '_trade_metrics', None)
        if s and t:
            metrics_text = (
                f"Return: {s['total_return']:+.2%}   Sharpe: {s['sharpe']:.2f}   "
                f"MaxDD: {s['max_dd_pct']:.2%}   Calmar: {s['calmar_ratio']:.2f}   "
                f"CushionMult: {s['profit_cushion_multiple']:.2f}\n"
                f"Trades: {t['total_trades']}   WinRate: {t['win_rate']:.2%}   "
                f"PnLRatio: {t['pnl_ratio']:.2f}   AvgPnL: ${t['avg_pnl']:,.2f}\n"
                f"SmoothR2: {s['smooth_r2']:.2f}   K-ratio: {s['smooth_k_ratio']:.2f}"
            )
            ax1.text(0.01, 0.97, metrics_text, transform=ax1.transAxes,
                     ha='left', va='top', fontsize=9,
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
