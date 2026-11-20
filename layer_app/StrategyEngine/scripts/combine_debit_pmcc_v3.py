"""
第三版，修正前两版各自的问题：

v1 (CSV拼接): 只按结算日实现盈亏画曲线，完全没有持仓期间的逐bar浮动盈亏(MTM)，
    导致最大回撤被严重低估。

v2 (实时联合回测): 让PMCC的开仓判断实时读取Debit当前持仓数(len(debit._positions))，
    结果实测证明还是会超过8手上限——因为PMCC开仓那一刻"当下"够格，但PMCC仓位往往
    要持有几周甚至更久，这期间Debit完全可能再开新仓，而Debit的开仓判断从来不知道
    PMCC的存在。也就是说"任意时刻并发数<=8"这个约束，本质上需要在PMCC决定要不要
    开仓时就"预留"出它整个持仓期间Debit可能达到的峰值容量——而PMCC自己要持有多久
    是要等它实际跑完（到期/滚仓/dip等）才知道的，属于非因果的信息，实时判断无论
    如何做不到。

v3 (本版，正确做法): 两步走——
  1. Debit独立跑一遍(max_positions=4，不依赖PMCC，结果必然正确)，得到它完整的
     持仓时间线 + 逐bar MTM序列(_mtm_pnl_series，已经是它真实持有的全部仓位)。
  2. PMCC独立跑一遍(max_positions=0，无并发限制)，得到信号本身能生成的全部候选
     交易的完整生命周期。同时通过monkeypatch _calc_mtm_pnl，在跑的过程中额外记录
     "每个position_id在每根bar上的MTM贡献"（而不仅仅是全部持仓加总的一个数），
     这样事后才能只取"最终被接受的那些仓位"的MTM贡献，不会把被拒绝仓位的浮动
     盈亏也算进组合曲线里。
  然后做一次事后(post-hoc)的保守接受/拒绝判断：对每笔PMCC候选交易，检查它
  "整个持仓区间内"Debit出现过的最大并发数 + 已接受PMCC出现过的最大并发数 + 1
  是否 <= 8——这是非因果判断(需要提前知道两边完整的时间线)，但这正是唯一能
  保证约束在任意时刻都成立的办法，之前v2已经证明"只看当下"是不够的。
  最后把Debit的完整MTM + 被接受PMCC仓位各自的MTM贡献相加，得到组合策略真正
  含有逐bar浮动盈亏的资金曲线。
"""
import pickle
import sys

import pandas as pd
import matplotlib.pyplot as plt

sys.path.insert(0, '/Users/fantang/CLionProjects/MyTrader/layer_app/StrategyEngine/src')

from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.core.strategy_pool import make_pool, get_pool, reset_pool
from qib_trader.core.broker import make_broker, get_broker, reset_broker
from qib_trader.core.engine import BacktestEngine
from qib_trader.utils.config_loader import get_strategies_config
from qib_trader.utils.stats import _build_bar_df, _build_trade_df, _calc_equity, _calc_summary, _print_report

ROOT       = '/Users/fantang/CLionProjects/MyTrader/layer_app/StrategyEngine'
BARS_PKL   = f'{ROOT}/datadb/SMH_M5_20240101_20260804.pkl'
MULTIPLIER = 100.0
INITIAL_CAPITAL = 100_000.0
TOTAL_SLOTS = 8
DEBIT_SLOTS = 4


def _get_config(class_name: str) -> dict:
    for s in get_strategies_config():
        if s['class'] == class_name:
            return s
    raise ValueError(f"strategies.yaml 中找不到 {class_name}")


def _run_solo(class_name: str, max_positions: int, bars: list, track_per_position_mtm: bool):
    """独立跑一个策略，返回 (strategy实例, per_position_mtm字典或None)。"""
    make_pool(); make_broker(); reset_pool(); reset_broker()
    conf = _get_config(class_name)
    strategy = FactoryStrategy.create_strategy(conf)
    strategy.max_positions = max_positions

    per_position_mtm: dict = {}
    if track_per_position_mtm:
        def _tracking_calc_mtm(bar):
            S, total = bar.close_price, 0.0
            for p in strategy._positions:
                if p['phase'] == 'spread':
                    pnl = p['net_credit'] - strategy._spread_curr_price(bar, p, S)
                elif p.get('dip_waiting', False):
                    pnl = (strategy._stock_leg_value(bar, p, S)
                          + p['cc_spread_credit'] + p['prev_premiums'])
                else:
                    pnl, _ = strategy._cc_total_pnl(bar, p)
                per_position_mtm.setdefault(p['position_id'], {})[bar.datetime] = pnl
                total += pnl
            return total
        strategy._calc_mtm_pnl = _tracking_calc_mtm

    get_pool().add_strategy(strategy)
    BacktestEngine().run_backtest(bars)
    return strategy, (per_position_mtm if track_per_position_mtm else None)


def _max_open_count_during(positions: list, start, end) -> int:
    events = []
    for p in positions:
        if p['entry_dt'] < end and p['settlement_dt'] > start:
            lo = max(p['entry_dt'], start)
            hi = min(p['settlement_dt'], end)
            events.append((lo, 1))
            events.append((hi, -1))
    events.sort(key=lambda x: (x[0], -x[1]))
    peak = cur = 0
    for _, d in events:
        cur += d
        peak = max(peak, cur)
    return peak


def main() -> None:
    with open(BARS_PKL, 'rb') as f:
        bars = pickle.load(f)

    print("=== 1) Debit独立回测 (max_positions=4) ===")
    debit_strategy, _ = _run_solo('UTurnBSync_DebitCallSpreadEx', DEBIT_SLOTS, bars, track_per_position_mtm=False)
    debit_positions = debit_strategy._closed_positions
    print(f"Debit平仓次数: {len(debit_positions)}")

    print("\n=== 2) PMCC独立回测 (max_positions=0，无限制，含逐position MTM追踪) ===")
    pmcc_strategy, pmcc_position_mtm = _run_solo('UTurnBSync_CreditPutSpreadPMCC', 0, bars, track_per_position_mtm=True)
    pmcc_candidates = pmcc_strategy._closed_positions
    print(f"PMCC候选交易总数: {len(pmcc_candidates)}")

    print("\n=== 3) 事后保守接受/拒绝(按候选交易整个持仓区间预留容量) ===")
    candidates_sorted = sorted(pmcc_candidates, key=lambda p: p['entry_dt'])
    accepted, rejected = [], []
    for cand in candidates_sorted:
        window_start, window_end = cand['entry_dt'], cand['settlement_dt']
        debit_peak = _max_open_count_during(debit_positions, window_start, window_end)
        pmcc_peak  = _max_open_count_during(accepted, window_start, window_end)
        if debit_peak + pmcc_peak + 1 <= TOTAL_SLOTS:
            accepted.append(cand)
        else:
            rejected.append(cand)
    print(f"接受: {len(accepted)}  拒绝: {len(rejected)}")

    combined_closed = debit_positions + accepted
    events = sorted(
        [(p['entry_dt'], 1) for p in combined_closed] + [(p['settlement_dt'], -1) for p in combined_closed])
    peak, cur = 0, 0
    for _, d in events:
        cur += d
        peak = max(peak, cur)
    print(f"组合最大并发校验: {peak} (应该<=8)")
    assert peak <= TOTAL_SLOTS

    print("\n=== 4) 合并MTM，构建含真实浮动盈亏的资金曲线 ===")
    accepted_ids = {p['position_id'] for p in accepted}
    combined_mtm: dict = dict(debit_strategy._mtm_pnl_series)   # Debit的MTM全量采纳
    for pid, series in pmcc_position_mtm.items():
        if pid not in accepted_ids:
            continue   # 被拒绝的候选交易，它的浮动盈亏不计入组合曲线
        for dt, v in series.items():
            combined_mtm[dt] = combined_mtm.get(dt, 0.0) + v

    bar_df = _build_bar_df(bars)

    # _calc_equity 要求 options_mode=True 时 trade_df 非空才会走 lifecycle 分支，
    # 这里没有真实broker trade记录（两次独立回测各自用了自己的broker实例），
    # 直接构造一行占位trade即可触发该分支，内容不影响结果（lifecycle_pnl_by_bar
    # 只用 closed_positions，不用 trade_df 本身的数值）。
    from qib_trader.core.models import TradeData, Direction
    placeholder_trade = TradeData(codeId='SMH', order_id='x', trade_id='x', strategy_id='x',
                                  direction=Direction.LONG, price=0.0, volume=1,
                                  datetime=bar_df.index[0])
    trade_df = _build_trade_df([placeholder_trade])

    bar_df = _calc_equity(bar_df, trade_df, INITIAL_CAPITAL, MULTIPLIER,
                          options_mode=True, mtm_pnl_series=combined_mtm,
                          closed_positions=combined_closed)

    summary = _calc_summary(bar_df, INITIAL_CAPITAL, combined_closed)
    trade_metrics = {
        'total_trades': len(combined_closed),
        'win_rate': sum(1 for p in combined_closed if p['final_pnl'] > 0) / len(combined_closed),
        'pnl_ratio': 0.0,
        'avg_pnl': sum(p['final_pnl'] for p in combined_closed) / len(combined_closed) * MULTIPLIER,
    }
    _print_report(summary, trade_metrics, INITIAL_CAPITAL)

    plt.style.use("ggplot")
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)
    fig.suptitle("DebitCallSpreadEx(4 slots) + PMCC(overflow, total<=8, window-reserved) - with real MTM",
                fontsize=12, fontweight="bold")
    ax1.plot(bar_df.index, bar_df["equity"], color="#2c3e50", lw=1.5)
    ax1.set_title("Equity Curve", fontsize=14, fontweight="bold")
    ax1.set_ylabel("Account Value (USD)")
    metrics_text = (
        f"Return: {summary['total_return']:+.2%}   Sharpe: {summary['sharpe']:.2f}   "
        f"MaxDD: {summary['max_dd_pct']:.2%}   Calmar: {summary['calmar_ratio']:.2f}\n"
        f"Trades: {trade_metrics['total_trades']} (Debit={len(debit_positions)} "
        f"PMCC accepted={len(accepted)} rejected={len(rejected)})   "
        f"WinRate: {trade_metrics['win_rate']:.2%}\n"
        f"SmoothR2: {summary['smooth_r2']:.2f}   K-ratio: {summary['smooth_k_ratio']:.2f}"
    )
    ax1.text(0.01, 0.97, metrics_text, transform=ax1.transAxes, ha='left', va='top', fontsize=9,
             bbox=dict(boxstyle='round,pad=0.4', facecolor='white', alpha=0.85, edgecolor='#cccccc'))
    ax2.fill_between(bar_df.index, bar_df["drawdown"] * 100, 0, facecolor="#e74c3c", alpha=0.5)
    ax2.set_title("Drawdown", fontsize=14, fontweight="bold")
    ax2.set_ylabel("Percentage (%)")
    plt.tight_layout()
    save_path = f'{ROOT}/output1/SMH/deallist/Combined_DebitEx4_PMCCOverflow_v3.png'
    fig.savefig(save_path, dpi=150, bbox_inches='tight')
    print(f"\n图表已保存: {save_path}")


if __name__ == "__main__":
    main()
