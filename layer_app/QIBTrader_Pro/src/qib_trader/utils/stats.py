import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from typing import List, Dict, Any
from dataclasses import asdict

# 导入核心数据模型
from qib_trader.core.models import TradeData, BarData, Direction


class PerformanceAnalyzer:
    """
    量化回测绩效分析器 (增强版)
    支持胜率、盈亏比、平均盈亏及收益风险比计算。
    """

    def __init__(
            self,
            trades: List[TradeData],
            bars: List[BarData],
            initial_capital: float = 100000.0,
            contract_multiplier: float = 1.0  # 默认为 1，由外部注入
    ):
        self.trades = trades
        self.bars = bars
        self.initial_capital = initial_capital
        self.multiplier = contract_multiplier

    def calculate_performance(self) -> pd.DataFrame:
        """利用 Pandas 向量化计算净值曲线并提取交易统计"""
        if not self.trades:
            print("警告: 引擎中无成交记录，无法计算绩效报告。")
            return pd.DataFrame()

        # 1. 基础行情数据转换
        df = pd.DataFrame([asdict(b) for b in self.bars])
        df['datetime'] = pd.to_datetime(df['datetime'])
        df.set_index('datetime', inplace=True)
        # 仅保留计算字段，确保兼容性
        df = df[['open_price', 'high_price', 'low_price', 'close_price', 'volume']]

        # 2. 交易记录处理
        trade_df = pd.DataFrame([asdict(t) for t in self.trades])
        trade_df['datetime'] = pd.to_datetime(trade_df['datetime'])

        # 3. 仓位与净值计算 (Time-series Basis)
        def get_pos_change(row):
            match row.direction:
                case Direction.LONG:
                    return row.volume
                case Direction.SHORT:
                    return -row.volume
                case _:
                    return 0.0

        trade_df['pos_change'] = trade_df.apply(get_pos_change, axis=1)
        df['pos_change'] = trade_df.groupby('datetime')['pos_change'].sum().reindex(df.index).fillna(0)
        df['position'] = df['pos_change'].cumsum()

        # PnL 计算：昨仓 * 价格变动 * 乘数
        df['pnl'] = df['position'].shift(1).fillna(0) * df['close_price'].diff().fillna(0) * self.multiplier
        df['equity'] = self.initial_capital + df['pnl'].cumsum()
        df['max_equity'] = df['equity'].expanding().max()
        df['drawdown'] = (df['equity'] - df['max_equity']) / df['max_equity']

        # 4. 提取平仓统计 (Trade-based Basis) [新增]
        trade_results = self._calculate_trade_metrics(trade_df)

        # 5. 打印增强版报告
        self._show_report(df, trade_results)

        return df

    def _calculate_trade_metrics(self, trade_df: pd.DataFrame) -> Dict[str, Any]:
        """
        计算基于每一笔完整交易(Round-trip)的指标
        """
        results = {
            "win_rate": 0.0, "pnl_ratio": 0.0, "avg_pnl": 0.0,
            "win_count": 0, "loss_count": 0, "total_trades": 0
        }

        # 简单实现：逐笔抵消逻辑（假设每次 1 手，适合目前的双均线测试）
        pnls = []
        pos = 0
        entry_price = 0.0

        for _, row in trade_df.iterrows():
            if pos == 0:  # 开仓
                entry_price = row.price
                pos = 1 if row.direction == Direction.LONG else -1
            else:  # 平仓
                pnl = (row.price - entry_price) * self.multiplier if pos > 0 else (
                                                                                              entry_price - row.price) * self.multiplier
                pnls.append(pnl)
                pos = 0

        if pnls:
            pnl_series = pd.Series(pnls)
            wins = pnl_series[pnl_series > 0]
            losses = pnl_series[pnl_series < 0]

            results["total_trades"] = len(pnls)
            results["win_count"] = len(wins)
            results["loss_count"] = len(losses)
            results["win_rate"] = len(wins) / len(pnls)
            results["avg_pnl"] = pnl_series.mean()

            avg_win = wins.mean() if not wins.empty else 0
            avg_loss = abs(losses.mean()) if not losses.empty else 1.0
            results["pnl_ratio"] = avg_win / avg_loss

        return results

    def _show_report(self, df: pd.DataFrame, trades: Dict[str, Any]) -> None:
        """打印包含胜率和盈亏比的增强报告"""
        total_net_pnl = df['equity'].iloc[-1] - self.initial_capital
        total_return = (df['equity'].iloc[-1] / self.initial_capital) - 1
        max_dd_pct = df['drawdown'].min()
        max_dd_amount = (df['max_equity'] - df['equity']).max()

        # 夏普比率 (日频数据假设)
        daily_returns = df['equity'].pct_change().fillna(0)
        std = daily_returns.std()
        sharpe = (daily_returns.mean() / std) * np.sqrt(252) if std != 0 else 0

        # 收益风险比 (Recovery Factor)
        recovery_factor = total_net_pnl / max_dd_amount if max_dd_amount != 0 else 0

        print("\n" + "█" * 44)
        print(f"┃ {'QUANT ENHANCED REPORT':^40} ┃")
        print("█" * 44)
        print(f"┃ 初始资金:   ${self.initial_capital:,.2f}")
        print(f"┃ 最终净值:   ${df['equity'].iloc[-1]:,.2f} ({total_return:+.2%})")
        print(f"┃ 最大回撤:   {max_dd_pct:.2%} (-${max_dd_amount:,.2f})")
        print(f"┃ 夏普比率:   {sharpe:.2f}")
        print(f"┃ 收益风险比:  {recovery_factor:.2f}")
        print("┣" + "━" * 42 + "┫")
        print(f"┃ 总平仓次数:  {trades['total_trades']}")
        print(f"┃ 胜率:       {trades['win_rate']:.2%}")
        print(f"┃ 盈亏比:     {trades['pnl_ratio']:.2f}")
        print(f"┃ 单笔平均盈亏: ${trades['avg_pnl']:,.2f}")
        print("█" * 44 + "\n")

    def plot_result(self, df: pd.DataFrame):
        """保持原有可视化逻辑，增加收益曲线显示"""
        if df.empty: return
        plt.style.use('ggplot')
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)
        ax1.plot(df.index, df['equity'], color='#2c3e50', lw=2, label='Equity Curve')
        ax1.set_title('Strategic Performance: Equity Curve', fontsize=14, fontweight='bold')
        ax1.set_ylabel('Account Value (USD)')
        ax1.legend(loc='upper left')
        ax2.fill_between(df.index, df['drawdown'] * 100, 0, facecolor='#e74c3c', alpha=0.5, label='Drawdown %')
        ax2.set_title('Risk Profile: Drawdown', fontsize=14, fontweight='bold')
        ax2.set_ylabel('Percentage (%)')
        plt.tight_layout()
        plt.show()