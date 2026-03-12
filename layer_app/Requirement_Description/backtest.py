#!/usr/bin/env python3
"""
NQ 期货回测脚本
策略1：20日高低点突破策略（M15）
策略2：期权 Bull Call Spread 策略（M15 + D1）
"""

import os
import math
import pandas as pd
import numpy as np
from scipy.stats import norm
from datetime import datetime, timedelta

# ---------------------------------------------------------------------------
# 路径配置
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
M15_CSV = os.path.join(SCRIPT_DIR, "NQ_kline_M15.csv")
D1_CSV  = os.path.join(SCRIPT_DIR, "NQ_kline_D1.csv")

INITIAL_CAPITAL = 200_000.0
NQ_POINT_VALUE  = 20.0        # 每点 $20

# ---------------------------------------------------------------------------
# Black-Scholes 工具
# ---------------------------------------------------------------------------

def bs_call_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Call 期权 Black-Scholes 价格（T 以年为单位）"""
    if T <= 0:
        return max(S - K, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)

# ---------------------------------------------------------------------------
# 数据加载
# ---------------------------------------------------------------------------

def load_m15() -> pd.DataFrame:
    df = pd.read_csv(M15_CSV)
    df.columns = df.columns.str.strip()
    df['time'] = pd.to_datetime(df['time'], format='%Y%m%d %H:%M:%S')
    df['tradeDay'] = df['tradeDay'].astype(str).str.strip()
    # 将字符串列转为数值
    for col in ['dif', 'dea', 'macd', 'atr', 'ma5', 'ma20', 'ma60', 'ma200']:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    df['isUTurn'] = pd.to_numeric(df['isUTurn'], errors='coerce').fillna(0).astype(int)
    df['divType'] = df['divType'].str.strip()
    df = df.sort_values('time').reset_index(drop=True)
    return df


def load_d1() -> pd.DataFrame:
    df = pd.read_csv(D1_CSV)
    df.columns = df.columns.str.strip()
    df['time'] = pd.to_datetime(df['time'], format='%Y%m%d %H:%M:%S')
    df['tradeDay'] = df['tradeDay'].astype(str).str.strip()
    for col in ['high', 'low', 'close', 'open']:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    df = df.sort_values('time').reset_index(drop=True)
    return df

# ---------------------------------------------------------------------------
# 绩效报告
# ---------------------------------------------------------------------------

def print_performance(name: str, trades: list, initial_capital: float):
    """
    trades: list of dicts with key 'pnl'
    """
    print("=" * 60)
    print(f"  策略：{name}")
    print("=" * 60)

    if not trades:
        print("  无已关闭交易记录。")
        print()
        return

    pnls = [t['pnl'] for t in trades]
    wins  = [p for p in pnls if p > 0]
    losses = [p for p in pnls if p <= 0]

    total_pnl    = sum(pnls)
    total_return = total_pnl / initial_capital
    n_trades     = len(pnls)
    win_rate     = len(wins) / n_trades if n_trades else 0.0
    avg_win      = sum(wins)  / len(wins)   if wins   else 0.0
    avg_loss     = sum(losses)/ len(losses) if losses else 0.0
    profit_factor = abs(avg_win / avg_loss) if avg_loss != 0 else float('inf')

    # 最大回撤（基于权益曲线）
    equity = initial_capital
    peak   = equity
    max_dd = 0.0
    for p in pnls:
        equity += p
        peak = max(peak, equity)
        dd = peak - equity
        max_dd = max(max_dd, dd)

    print(f"  总盈亏         : ${total_pnl:>12,.2f}")
    print(f"  收益率         : {total_return:>11.2%}")
    print(f"  交易次数       : {n_trades:>12d}")
    print(f"  胜率           : {win_rate:>11.2%}")
    print(f"  平均单笔盈利   : ${avg_win:>12,.2f}")
    print(f"  平均单笔亏损   : ${avg_loss:>12,.2f}")
    print(f"  盈亏比         : {profit_factor:>12.2f}")
    print(f"  最大回撤       : ${max_dd:>12,.2f}")
    print()


def print_trade_details(name: str, trades: list):
    print(f"--- {name} 交易明细 ---")
    for i, t in enumerate(trades, 1):
        entry = t.get('entry_time', '')
        exit_ = t.get('exit_time', '')
        pnl   = t['pnl']
        print(f"  #{i:3d}  进场:{entry}  出场:{exit_}  P&L:${pnl:>10,.2f}")
    print()

# ---------------------------------------------------------------------------
# 策略1：20日高低点突破策略
# ---------------------------------------------------------------------------

def run_strategy1(df: pd.DataFrame, verbose: bool = False) -> list:
    """
    规则：
    - 无持仓 且 当天未开过仓
    - high > prev20_high 且 08:00 <= bar.time < 15:00
    - 以 prev20_high 作为入场价开多
    - 任何时候 low < prev20_low 则以 prev20_low 平仓
    - P&L = (exit - entry) * NQ_POINT_VALUE
    """
    trades = []
    position = None          # {'entry_price', 'entry_time', 'trade_day'}
    opened_trade_days = set()  # 记录已开过仓的交易日

    for i in range(20, len(df)):
        bar = df.iloc[i]
        prev20_high = df['high'].iloc[i - 20:i].max()
        prev20_low  = df['low'].iloc[i - 20:i].min()
        trade_day   = bar['tradeDay']
        bar_time    = bar['time']
        bar_hour    = bar_time.hour + bar_time.minute / 60.0

        # ---- 平仓检查（优先） ----
        if position is not None:
            if bar['low'] < prev20_low:
                exit_price = prev20_low
                pnl = (exit_price - position['entry_price']) * NQ_POINT_VALUE
                trade_rec = {
                    'entry_time':  position['entry_time'],
                    'exit_time':   bar_time,
                    'entry_price': position['entry_price'],
                    'exit_price':  exit_price,
                    'pnl':         pnl,
                }
                trades.append(trade_rec)
                position = None

        # ---- 开仓检查 ----
        if position is None and trade_day not in opened_trade_days:
            if bar['high'] > prev20_high and 8.0 <= bar_hour < 15.0:
                entry_price = prev20_high
                position = {
                    'entry_price': entry_price,
                    'entry_time':  bar_time,
                    'trade_day':   trade_day,
                }
                opened_trade_days.add(trade_day)

    # 如果回测结束时仍有持仓，强制平仓（用最后一根 bar 的收盘价）
    if position is not None:
        last_bar   = df.iloc[-1]
        exit_price = last_bar['close']
        pnl = (exit_price - position['entry_price']) * NQ_POINT_VALUE
        trades.append({
            'entry_time':  position['entry_time'],
            'exit_time':   last_bar['time'],
            'entry_price': position['entry_price'],
            'exit_price':  exit_price,
            'pnl':         pnl,
            'note':        'force_close',
        })

    if verbose:
        print_trade_details("策略1", trades)

    return trades


# ---------------------------------------------------------------------------
# 策略2：期权 Bull Call Spread
# ---------------------------------------------------------------------------

OPTION_IV = 0.18
OPTION_R  = 0.04
OPTION_T_DAYS = 14  # 期权持续天数

# ==========================================
# 1. 基础工具函数
# ==========================================

def bs_call_price(S, K, T, r, sigma):
    """Black-Scholes 期权定价公式"""
    if T <= 0:
        return max(0.0, S - K)
    d1 = (np.log(S / K) + (r + 0.5 * sigma**2) * T) / (sigma * np.sqrt(T))
    d2 = d1 - sigma * np.sqrt(T)
    return S * norm.cdf(d1) - K * np.exp(-r * T) * norm.cdf(d2)

def _atm_strike(price):
    """行权价取最近的整百"""
    return round(price / 100.0) * 100.0

def _remaining_T(bar_time, expiry):
    """计算剩余时间（年）"""
    delta = (expiry - bar_time).total_seconds() / (365.25 * 24 * 3600)
    return max(delta, 0.0)


def run_strategy2(m15: pd.DataFrame, d1: pd.DataFrame, verbose: bool = False) -> list:
    """
    期权 Bull Call Spread 策略
    """
    # 构建 D1 查找表：以 tradeDay 字符串为键
    d1_dict = {}
    sorted_d1 = d1.sort_values('time').reset_index(drop=True)
    trade_days_d1 = sorted_d1['tradeDay'].tolist()

    for idx, row in sorted_d1.iterrows():
        d1_dict[row['tradeDay']] = row

    # 获取某交易日的"前一日"D1 数据
    def get_yesterday_d1(trade_day_str: str):
        try:
            pos = trade_days_d1.index(trade_day_str)
        except ValueError:
            return None
        if pos == 0:
            return None
        prev_day = trade_days_d1[pos - 1]
        return d1_dict.get(prev_day)

    trades  = []
    positions = []   # 当前所有持仓

    for i, bar in m15.iterrows():
        bar_time   = bar['time']
        trade_day  = bar['tradeDay']
        close      = bar['close']
        dif        = bar['dif']
        div_type   = bar['divType']
        is_uturn   = bar['isUTurn']

        yesterday = get_yesterday_d1(trade_day)

        # ---- 遍历所有未关闭持仓 ----
        still_open = []
        for pos in positions:
            if pos['closed']:
                trades.append({'entry_time': pos['entry_time'],
                               'exit_time':  bar_time,
                               'pnl':        pos['pnl']})
                continue

            expiry      = pos['expiry']
            k1          = pos['strike_k1']
            net_cost    = pos['premium_paid'] - pos.get('short_premium', 0.0)
            remaining_t = _remaining_T(bar_time, expiry)

            # a. 到期处理
            if bar_time >= expiry:
                if pos['status'] == 'long_call':
                    pnl = max(close - k1, 0.0) - pos['premium_paid']
                else:  # spread
                    k2  = pos['strike_k2']
                    pnl = max(close - k1, 0.0) - max(close - k2, 0.0) - net_cost
                pos['closed'] = True
                pos['pnl']    = pnl
                trades.append({'entry_time': pos['entry_time'],
                               'exit_time':  bar_time,
                               'pnl':        pnl})
                continue

            # b. 止损（仅 long_call）
            if pos['status'] == 'long_call':
                current_bs = bs_call_price(close, k1, remaining_t, OPTION_R, OPTION_IV)
                if current_bs <= 0.5 * pos['premium_paid']:
                    pnl = current_bs - pos['premium_paid']
                    pos['closed'] = True
                    pos['pnl']    = pnl
                    trades.append({'entry_time': pos['entry_time'],
                                   'exit_time':  bar_time,
                                   'pnl':        pnl})
                    continue

            # c. 止盈（仅 long_call）
            if pos['status'] == 'long_call':
                triggered = False

                # 顶部 uturn 止盈（优先）
                if div_type in ('Top', 'TopSub') and is_uturn == 1:
                    if (not pd.isna(dif)) and dif > 30 and yesterday is not None and bar['high'] > yesterday['high']:
                        k2         = _atm_strike(close)
                        short_prem = bs_call_price(close, k2, remaining_t, OPTION_R, OPTION_IV)
                        pos['status']        = 'spread'
                        pos['strike_k2']     = k2
                        pos['short_premium'] = short_prem
                        triggered = True

                # 超时止盈（72h 后）
                if not triggered:
                    elapsed = bar_time - pos['entry_time']
                    if elapsed > timedelta(hours=72):
                        if close > pos['entry_close']:
                            k2         = _atm_strike(close)
                            short_prem = bs_call_price(close, k2, remaining_t, OPTION_R, OPTION_IV)
                            pos['status']        = 'spread'
                            pos['strike_k2']     = k2
                            pos['short_premium'] = short_prem
                        else:
                            current_bs = bs_call_price(close, k1, remaining_t, OPTION_R, OPTION_IV)
                            pnl = current_bs - pos['premium_paid']
                            pos['closed'] = True
                            pos['pnl']    = pnl
                            trades.append({'entry_time': pos['entry_time'],
                                           'exit_time':  bar_time,
                                           'pnl':        pnl})
                            continue

            still_open.append(pos)

        positions = still_open

        # ---- 开仓检查 ----
        if yesterday is not None:
            if (not pd.isna(bar['low'])) and bar['low'] < yesterday['low']:
                if (not pd.isna(dif)) and dif < -30:
                    if div_type in ('Bottom', 'BottomSub') and is_uturn == 1:
                        k1      = _atm_strike(close)
                        T       = OPTION_T_DAYS / 365.0
                        expiry  = bar_time + timedelta(days=OPTION_T_DAYS)
                        premium = bs_call_price(close, k1, T, OPTION_R, OPTION_IV)
                        positions.append({
                            'entry_time':    bar_time,
                            'entry_close':   close,
                            'strike_k1':     k1,
                            'expiry':        expiry,
                            'premium_paid':  premium,
                            'status':        'long_call',
                            'strike_k2':     None,
                            'short_premium': 0.0,
                            'closed':        False,
                            'pnl':           0.0,
                        })

    # 回测结束，强制平仓所有剩余持仓
    last_bar  = m15.iloc[-1]
    last_time = last_bar['time']
    last_close= last_bar['close']

    for pos in positions:
        if pos['closed']:
            trades.append({'entry_time': pos['entry_time'],
                           'exit_time':  last_time,
                           'pnl':        pos['pnl']})
            continue
        k1          = pos['strike_k1']
        remaining_t = _remaining_T(last_time, pos['expiry'])
        net_cost    = pos['premium_paid'] - pos.get('short_premium', 0.0)

        if pos['status'] == 'long_call':
            current_bs = bs_call_price(last_close, k1, remaining_t, OPTION_R, OPTION_IV)
            pnl = current_bs - pos['premium_paid']
        else:
            k2  = pos['strike_k2']
            pnl = (bs_call_price(last_close, k1, remaining_t, OPTION_R, OPTION_IV)
                   - bs_call_price(last_close, k2, remaining_t, OPTION_R, OPTION_IV)
                   - net_cost)
        trades.append({'entry_time': pos['entry_time'],
                       'exit_time':  last_time,
                       'pnl':        pnl})

    if verbose:
        print_trade_details("策略2", trades)

    return trades


# ---------------------------------------------------------------------------
# 主程序
# ---------------------------------------------------------------------------

def main():
    print("加载数据...")
    m15 = load_m15()
    d1  = load_d1()
    print(f"  M15 数据：{len(m15)} 根 bar，时间范围 {m15['time'].iloc[0]} ~ {m15['time'].iloc[-1]}")
    print(f"  D1  数据：{len(d1)} 根 bar，时间范围 {d1['time'].iloc[0]} ~ {d1['time'].iloc[-1]}")
    print()

    # 是否打印交易明细
    VERBOSE = False  # 改为 True 可查看每笔交易

    # ---- 策略1 ----
    print("运行策略1（突破策略）...")
    trades1 = run_strategy1(m15, verbose=VERBOSE)
    print_performance("策略1：20日突破做多（M15）", trades1, INITIAL_CAPITAL)

    # ---- 策略2 ----
    print("运行策略2（Bull Call Spread）...")
    trades2 = run_strategy2(m15, d1, verbose=VERBOSE)
    print_performance("策略2：期权 Bull Call Spread（M15+D1）", trades2, INITIAL_CAPITAL)


if __name__ == "__main__":
    main()
