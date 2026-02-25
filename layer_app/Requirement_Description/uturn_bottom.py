#!/usr/bin/env python3
"""
NQ 期货期权回测脚本 (最终优化版)
策略：底部 UTurn 入场 + 防御型锁定止损 + 宽幅锁定止盈

1、使用NQ_kline_M15.csv中的数据做交易。其中divType为Bottom或者BottomSub，且isUturn等于1，这种形态叫做底部uturn;
   divType为Top或者TopSub，且isUturn等于1，这种形态叫做顶部uturn。
2、涉及到的期权价格都由bs公式模拟计算取得。预设nq期权的iv恒定为18%，无风险收益率为4%
3、如果当前bar的最低价比昨日最低价更低（昨日最低价通过NQ_kline_D1.csv文件取得），且dif < -30,且出现底部uturn，则买入14天后到期的atm call。
   且记录当前bar的收盘价为开仓bar收盘价。
4、开仓之后，如果当前bar出现顶部uturn且dif > 30,且当前bar最高价比昨日最高价更高，则卖出相同到期日的call，行权价为当前bar收盘价 + 100。形成bull call spread，锁住。形成止盈。并持有这个组合直到期权到期为止。
5、开仓之后，如果行情下跌使得期权权利金损失了一半，就卖出相同到期日的call，行权价为开仓bar收盘价 + 100，锁住形成bull call spread 止损。
6、开仓之后，过了72小时，既没有被止损，也没有止盈，则看当前bar的收盘价格是否比开仓bar收盘价更高，
   如果更高，则卖出相同到期日的call，行权价是当前bar + 100。形成bull call spread，锁住。形成止盈。并持有这个组合直到期权到期为止；
   如果更低，则卖出相同到期日的call，行权价为开仓bar收盘价 + 100。
7、一旦开仓的long call被止损平仓，或者形成bull call spread锁定，就释放开仓空间，如果有新的开仓信号出来，就可以继续开仓了。



"""

import pandas as pd
import numpy as np
from scipy.stats import norm
from datetime import datetime, timedelta
import math

# ---------------------------------------------------------------------------
# 1. 基础配置与参数
# ---------------------------------------------------------------------------
INITIAL_CAPITAL = 200_000.0
NQ_POINT_VALUE = 20.0  # NQ 每点价值 $20
OPTION_IV = 0.24  # 预设恒定 IV 18%
OPTION_R = 0.04  # 无风险利率 4%
OPTION_T_DAYS = 14  # 初始到期天数 14天


# ---------------------------------------------------------------------------
# 2. 辅助工具函数
# ---------------------------------------------------------------------------

def bs_call_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """欧式 Call 期权 Black-Scholes 定价公式 (T以年为单位)"""
    if T <= 0:
        return max(S - K, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


def _remaining_T(bar_time: datetime, expiry: datetime) -> float:
    """计算剩余到期时间（年化）"""
    delta = (expiry - bar_time).total_seconds() / (365.25 * 24 * 3600)
    return max(delta, 0.0)


def _atm_strike(close: float) -> float:
    """行权价取最近的整百位"""
    return round(close / 100.0) * 100.0


# ---------------------------------------------------------------------------
# 3. 策略核心引擎
# ---------------------------------------------------------------------------

def run_strategy(m15_path: str, d1_path: str):
    # --- A. 数据加载与预处理 ---
    m15 = pd.read_csv(m15_path)
    d1 = pd.read_csv(d1_path)
    m15['time'] = pd.to_datetime(m15['time'])
    d1['time'] = pd.to_datetime(d1['time'])

    # 提取昨日高低点并缝合到 M15 数据中 (提升回测速度)
    d1_temp = d1.sort_values('time').copy()
    d1_temp['prev_high'] = d1_temp['high'].shift(1)
    d1_temp['prev_low'] = d1_temp['low'].shift(1)
    d1_temp['tradeDay'] = d1_temp['tradeDay'].astype(str)

    m15_temp = m15.copy()
    m15_temp['tradeDay'] = m15_temp['tradeDay'].astype(str)
    m15_temp = m15_temp.merge(d1_temp[['tradeDay', 'prev_high', 'prev_low']], on='tradeDay', how='left')

    trades = []
    positions = []  # 存储活跃持仓

    # --- B. 主循环 ---
    for i, bar in m15_temp.iterrows():
        bar_time = bar['time']
        close = bar['close']
        dif = bar['dif']
        div_type = bar['divType']
        is_uturn = bar['isUTurn']
        y_high, y_low = bar['prev_high'], bar['prev_low']

        # 1. 管理现有持仓
        next_gen_positions = []
        for pos in positions:
            rem_t = _remaining_T(bar_time, pos['expiry'])

            # a. 到期结算
            if bar_time >= pos['expiry']:
                if pos['status'] == 'long_call':
                    pnl = max(close - pos['strike_k1'], 0.0) - pos['premium_paid']
                else:  # 组合结算 (Spread)
                    cost = pos['premium_paid'] - pos['short_premium']
                    pnl = (max(close - pos['strike_k1'], 0.0) - max(close - pos['strike_k2'], 0.0)) - cost
                trades.append({'pnl': pnl})
                continue

            # b. 单腿 Call 的中途管理
            if pos['status'] == 'long_call':
                curr_val = bs_call_price(close, pos['strike_k1'], rem_t, OPTION_R, OPTION_IV)

                # --- [新逻辑] 防御型止损锁定 ---
                triggered_sl = (curr_val <= 0.5 * pos['premium_paid'])
                triggered_72h_loss = (
                            bar_time >= pos['entry_time'] + timedelta(hours=72) and close <= pos['entry_close'])

                if triggered_sl or triggered_72h_loss:
                    # 不平仓，转为 Spread: 卖出行权价 = 开仓价 + 100
                    k2 = pos['entry_close'] + 100.0
                    pos['short_premium'] = bs_call_price(close, k2, rem_t, OPTION_R, OPTION_IV)
                    pos['strike_k2'], pos['status'] = k2, 'spread'
                    next_gen_positions.append(pos)
                    continue

                # --- [新逻辑] 宽幅止盈锁定 ---
                is_top = (div_type in ('Top', 'TopSub')) and (is_uturn == 1)
                is_tp_signal = (is_top and dif > 30 and (not pd.isna(y_high)) and bar['high'] > y_high)
                is_72h_gain = (bar_time >= pos['entry_time'] + timedelta(hours=72) and close > pos['entry_close'])

                if is_tp_signal or is_72h_gain:
                    # 卖出行权价 = 当前平仓 Bar 收盘价 + 100
                    k2 = close + 100.0
                    pos['short_premium'] = bs_call_price(close, k2, rem_t, OPTION_R, OPTION_IV)
                    pos['strike_k2'], pos['status'] = k2, 'spread'
                    next_gen_positions.append(pos)
                    continue

            next_gen_positions.append(pos)
        positions = next_gen_positions

        # 2. 开仓检查
        # 规则：只要没有 active long_call 即可开新单 (已转为 spread 的不占用名额)
        has_active_long = any(p['status'] == 'long_call' for p in positions)

        if not has_active_long and (not pd.isna(y_low)):
            is_bottom = (div_type in ('Bottom', 'BottomSub')) and (is_uturn == 1)
            if bar['low'] < y_low and dif < -30 and is_bottom:
                k1 = _atm_strike(close)
                premium = bs_call_price(close, k1, OPTION_T_DAYS / 365.25, OPTION_R, OPTION_IV)
                positions.append({
                    'entry_time': bar_time, 'entry_close': close, 'strike_k1': k1,
                    'expiry': bar_time + timedelta(days=OPTION_T_DAYS),
                    'premium_paid': premium, 'status': 'long_call', 'short_premium': 0.0
                })

    return trades


# ---------------------------------------------------------------------------
# 4. 主程序入口与报表
# ---------------------------------------------------------------------------

def main():
    print("加载数据中...")
    # 请确保 NQ_kline_M15.csv 和 NQ_kline_D1.csv 在当前目录下
    results = run_strategy("NQ_kline_M15.csv", "NQ_kline_D1.csv")

    if results:
        df_res = pd.DataFrame(results)
        pnl_pts = df_res['pnl'].values
        pnl_usd = pnl_pts * NQ_POINT_VALUE

        # 统计指标计算
        total_pnl = np.sum(pnl_usd)
        win_mask = pnl_usd > 0
        win_rate = (np.sum(win_mask) / len(pnl_pts)) * 100

        avg_win_usd = np.mean(pnl_usd[win_mask])
        avg_loss_usd = np.abs(np.mean(pnl_usd[~win_mask]))
        pl_ratio = avg_win_usd / avg_loss_usd

        # 最大回撤计算
        equity_curve = INITIAL_CAPITAL + np.cumsum(pnl_usd)
        equity_curve = np.insert(equity_curve, 0, INITIAL_CAPITAL)
        running_max = np.maximum.accumulate(equity_curve)
        drawdowns = running_max - equity_curve
        max_dd_usd = np.max(drawdowns)
        max_dd_pct = (max_dd_usd / running_max[np.argmax(drawdowns)]) * 100

        print("\n" + "=" * 50)
        print(f"【策略2：期权 Bull Call Spread 最终版报告】")
        print("-" * 50)
        print(f"总盈亏 (USD):       ${total_pnl:,.2f}")
        print(f"收益率:             {total_pnl / INITIAL_CAPITAL * 100:.2f}%")
        print(f"交易次数:           {len(pnl_pts)}")
        print(f"胜率:               {win_rate:.2f}%")
        print(f"平均单笔盈利点数:    {np.mean(pnl_pts):.2f} 点")
        print(f"平均单笔盈亏 (USD):  ${np.mean(pnl_usd):,.2f}")
        print(f"盈亏比:             {pl_ratio:.2f}")
        print(f"最大回撤 (USD):     ${max_dd_usd:,.2f}")
        print(f"最大回撤 (%):       {max_dd_pct:.2f}%")
        print("=" * 50)
    else:
        print("回测期间未触发交易信号。")


if __name__ == "__main__":
    main()