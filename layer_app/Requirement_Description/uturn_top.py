#!/usr/bin/env python3
"""
NQ 期货期权回测脚本 - 策略 3 (甜点参数优化版)
逻辑：高位卖出 0.3 Delta Call + 底部买入 ATM Call 锁定 + 触发行权价止损


1、使用NQ_kline_M15.csv中的数据做交易。其中divType为Bottom或者BottomSub，且isUturn等于1，这种形态叫做底部uturn;
   divType为Top或者TopSub，且isUturn等于1，这种形态叫做顶部uturn。
2、涉及到的期权价格都由bs公式模拟计算取得。预设nq期权的iv恒定为18%，无风险收益率为4%
3、如果当前bar的最高价比昨日最高价更高（昨日最低价通过NQ_kline_D1.csv文件取得），且dif > 20,且出现顶部uturn，则short 28天后到期的delta值为0.3的call。
   且记录当前bar的收盘价为开仓bar收盘价。
4、开仓之后，如果当前bar出现底部uturn且dif < -30,且当前bar最低价比昨日最低价更低，则买入相同到期日的atm call。形成bull call spread，锁住。形成止盈。并持有这个组合直到期权到期为止。
5、开仓之后，如果行情上涨，达到了开仓call的行权价，就强制平掉short call止损。
6、一旦开仓的short call被止损平仓，或者形成bull call spread锁定，就释放开仓空间，如果有新的开仓信号出来，就可以继续开仓了。




"""

import pandas as pd
import numpy as np
from scipy.stats import norm
from datetime import datetime, timedelta
import math

# ---------------------------------------------------------------------------
# 1. 基础配置与甜点区参数
# ---------------------------------------------------------------------------
INITIAL_CAPITAL = 200_000.0
NQ_POINT_VALUE = 20.0
OPTION_IV = 0.24  # 预设恒定 IV
OPTION_R = 0.04  # 无风险利率
DELTA_TARGET = 0.3  # 卖出 Call 的目标 Delta
ENTRY_DIF = 20  # 优化后的入场 DIF 阈值
TP_DIF = -30  # 锁定组合的 DIF 阈值
T_DAYS = 28  # 优化后的到期天数 (28天表现远优于14天)


# ---------------------------------------------------------------------------
# 2. 辅助工具函数
# ---------------------------------------------------------------------------

def bs_call_price(S: float, K: float, T: float, r: float, sigma: float) -> float:
    """BS 期权定价"""
    if T <= 0:
        return max(S - K, 0.0)
    d1 = (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


def get_strike_from_delta(S: float, delta: float, T: float, r: float, sigma: float) -> float:
    """根据目标 Delta 逆推行权价 K (N(d1) = Delta)"""
    d1 = norm.ppf(delta)
    K = S / math.exp(d1 * sigma * math.sqrt(T) - (r + 0.5 * sigma ** 2) * T)
    return K


# ---------------------------------------------------------------------------
# 3. 策略引擎
# ---------------------------------------------------------------------------

def run_strategy3(m15_path: str, d1_path: str):
    # 数据加载
    m15 = pd.read_csv(m15_path)
    d1 = pd.read_csv(d1_path)
    m15['time'] = pd.to_datetime(m15['time'])
    d1['time'] = pd.to_datetime(d1['time'])

    # 数据缝合：合并昨日高低点
    d1_temp = d1.sort_values('time').copy()
    d1_temp['prev_high'] = d1_temp['high'].shift(1)
    d1_temp['prev_low'] = d1_temp['low'].shift(1)
    d1_temp['tradeDay'] = d1_temp['tradeDay'].astype(str)

    m15_temp = m15.copy()
    m15_temp['tradeDay'] = m15_temp['tradeDay'].astype(str)
    m15_temp = m15_temp.merge(d1_temp[['tradeDay', 'prev_high', 'prev_low']], on='tradeDay', how='left')

    trades = []
    positions = []  # 活跃持仓

    for i, bar in m15_temp.iterrows():
        bar_time = bar['time']
        close = bar['close']
        dif = bar['dif']
        div_type = bar['divType']
        is_uturn = bar['isUTurn']
        y_high, y_low = bar['prev_high'], bar['prev_low']

        # A. 持仓管理
        next_gen_positions = []
        for pos in positions:
            rem_t = (pos['expiry'] - bar_time).total_seconds() / (365.25 * 24 * 3600)
            rem_t = max(0, rem_t)

            # 1. 到期结算
            if bar_time >= pos['expiry']:
                if pos['status'] == 'short_call':
                    pnl = pos['premium_received'] - max(close - pos['strike_k1'], 0)
                else:  # 锁定为 Bull Call Spread (Long K2, Short K1)
                    net_cost = pos['long_premium'] - pos['premium_received']
                    pnl = (max(close - pos['strike_k2'], 0) - max(close - pos['strike_k1'], 0)) - net_cost
                trades.append({'pnl': pnl})
                continue

            # 2. 卖单管理 (止损与锁定)
            if pos['status'] == 'short_call':
                # 止损：收盘价达到行权价 K1
                if close >= pos['strike_k1']:
                    # 按当时 BS 价格买回平仓
                    buyback = bs_call_price(close, pos['strike_k1'], rem_t, OPTION_R, OPTION_IV)
                    trades.append({'pnl': pos['premium_received'] - buyback})
                    continue

                # 止盈锁定：出现底部形态转为 Spread
                is_bottom = (div_type in ('Bottom', 'BottomSub')) and (is_uturn == 1)
                if is_bottom and dif < TP_DIF and (not pd.isna(y_low)) and bar['low'] < y_low:
                    # 买入 ATM Call 作为 K2
                    pos['strike_k2'] = close
                    pos['long_premium'] = bs_call_price(close, close, rem_t, OPTION_R, OPTION_IV)
                    pos['status'] = 'spread'

            next_gen_positions.append(pos)
        positions = next_gen_positions

        # B. 开仓检查 (Delta 0.3 Short Call)
        has_active_short = any(p['status'] == 'short_call' for p in positions)
        if not has_active_short and (not pd.isna(y_high)):
            is_top = (div_type in ('Top', 'TopSub')) and (is_uturn == 1)
            if bar['high'] > y_high and dif > ENTRY_DIF and is_top:
                # 动态求解 0.3 Delta 对应的 K
                k1 = get_strike_from_delta(close, DELTA_TARGET, T_DAYS / 365.25, OPTION_R, OPTION_IV)
                premium = bs_call_price(close, k1, T_DAYS / 365.25, OPTION_R, OPTION_IV)
                positions.append({
                    'status': 'short_call',
                    'strike_k1': k1,
                    'premium_received': premium,
                    'expiry': bar_time + timedelta(days=T_DAYS),
                    'strike_k2': 0.0,
                    'long_premium': 0.0
                })

    return trades


# ---------------------------------------------------------------------------
# 4. 主程序入口与报表
# ---------------------------------------------------------------------------

def main():
    print("正在执行策略 3 (优化版) 回测...")
    raw_trades = run_strategy3("NQ_kline_M15.csv", "NQ_kline_D1.csv")

    if not raw_trades:
        print("未触发交易信号")
        return

    df_res = pd.DataFrame(raw_trades)
    pnl_pts = df_res['pnl'].values
    pnl_usd = pnl_pts * NQ_POINT_VALUE

    win_mask = pnl_usd > 0
    total_pnl = np.sum(pnl_usd)

    # 最大回撤计算
    equity_curve = INITIAL_CAPITAL + np.cumsum(pnl_usd)
    equity_curve = np.insert(equity_curve, 0, INITIAL_CAPITAL)
    running_max = np.maximum.accumulate(equity_curve)
    max_dd = np.max(running_max - equity_curve)

    print("\n" + "=" * 50)
    print(f"总盈亏 (USD):       ${total_pnl:,.2f}")
    print(f"收益率:             {total_pnl / INITIAL_CAPITAL * 100:.2f}%")
    print(f"交易次数:           {len(pnl_pts)}")
    print(f"胜率:               {np.sum(win_mask) / len(pnl_pts) * 100:.2f}%")
    print(f"平均单笔盈利点数:    {np.mean(pnl_pts):.2f} 点")
    print(f"盈亏比:             {np.mean(pnl_usd[win_mask]) / abs(np.mean(pnl_usd[~win_mask])): .2f}")
    print(f"最大回撤 (USD):     ${max_dd:,.2f}")
    print(f"最大回撤 (%):       {max_dd / INITIAL_CAPITAL * 100:.2f}%")
    print("=" * 50)


if __name__ == "__main__":
    main()