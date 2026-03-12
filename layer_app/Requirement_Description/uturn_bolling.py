"""


''''''
NQ 期货期权回测脚本 (最终优化版)

1、使用NQ_kline_M15.csv中的数据做交易。其中divType为Bottom或者BottomSub，且isUturn等于1，这种形态叫做底部uturn;
divType为Top或者TopSub，且isUturn等于1，这种形态叫做顶部uturn。
2、涉及到的期权价格都由bs公式模拟计算取得。预设nq期权的iv恒定为18%，无风险收益率为4%
3、dif < -20,且出现底部uturn，最低价比1小时布林带下轨更低，则买入14天后到期的atm call。
且记录当前bar的收盘价为开仓bar收盘价。
4、开仓之后，如果当前bar出现顶部uturn且dif > 20,且最高价高于1小时布林带上轨，则卖出相同到期日的atm call 形成bull call spread，锁住。形成止盈。并持有这个组合直到期权到期为止。


"""

import pandas as pd
import numpy as np
from scipy.stats import norm
from datetime import timedelta

# --- 核心 Black-Scholes 期权定价函数 ---
def bs_call_price(S, K, T, r, sigma):
    """计算欧式看涨期权价格"""
    if T <= 0: return max(S - K, 0.0)
    d1 = (np.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * np.sqrt(T))
    d2 = d1 - sigma * np.sqrt(T)
    return S * norm.cdf(d1) - K * np.exp(-r * T) * norm.cdf(d2)

# 1. 加载数据 (请确保文件在当前目录下)
# df_m15 = pd.read_csv('NQ_kline_M15.csv')
# df_h1 = pd.read_csv('NQ_kline_H1.csv')

# --- 假设数据已加载并预处理 (此处保留原逻辑) ---
df_m15['time'] = pd.to_datetime(df_m15['time'])
df_h1['time'] = pd.to_datetime(df_h1['time'])

# 2. 计算 1 小时级别布林带
df_h1 = df_h1.sort_values('time')
df_h1['sma20'] = df_h1['close'].rolling(20).mean()
df_h1['std20'] = df_h1['close'].rolling(20).std()
df_h1['bb_lower'] = df_h1['sma20'] - 2.0 * df_h1['std20']
df_h1['bb_upper'] = df_h1['sma20'] + 2.0 * df_h1['std20']

df_h1['bb_available_at'] = df_h1['time'] + timedelta(hours=1)
bb_lookup = df_h1[['bb_available_at', 'bb_lower', 'bb_upper']].dropna()

# 3. 数据合并
df = df_m15.sort_values('time').copy()
df = pd.merge_asof(df, bb_lookup, left_on='time', right_on='bb_available_at', direction='backward')

# 4. 回测参数设置
IV = 0.18
R = 0.04
T_DAYS = 14
NQ_VAL = 20
INIT_CAP = 200000

DIF_ENTRY = -20
DIF_EXIT = 20

trades = [] # 记录每笔交易的盈亏点数
positions = []

# 5. 执行循环
for i, bar in df.iterrows():
    curr_time = bar['time']
    close, low, high, bb_low, bb_high, dif, div, uturn = \
        bar['close'], bar['low'], bar['high'], bar['bb_lower'], bar['bb_upper'], bar['dif'], bar['divType'], bar['isUTurn']

    next_positions = []
    for pos in positions:
        rem_t = max(0, (pos['expiry'] - curr_time).total_seconds() / (365.25 * 24 * 3600))

        if curr_time >= pos['expiry']:
            pnl = (max(close - pos['k1'], 0) - (max(close - pos['k2'], 0) if pos['status'] == 'spread' else 0)) - \
                  (pos['premium_paid'] - pos.get('short_premium_received', 0))
            trades.append(pnl)
            continue

        if pos['status'] == 'long_call':
            curr_opt_price = bs_call_price(close, pos['k1'], rem_t, R, IV)
            if curr_opt_price <= 0.5 * pos['premium_paid']:
                trades.append(curr_opt_price - pos['premium_paid'])
                continue
            is_top_signal = (div in ['Top', 'TopSub']) and (uturn == 1)
            if is_top_signal and dif > DIF_EXIT and (not pd.isna(bb_high)) and high > bb_high:
                pos['k2'] = close
                pos['short_premium_received'] = bs_call_price(close, pos['k2'], rem_t, R, IV)
                pos['status'] = 'spread'
                next_positions.append(pos)
                continue
            if curr_time >= pos['entry_time'] + timedelta(hours=72):
                if close > pos['entry_close']:
                    pos['k2'] = close
                    pos['short_premium_received'] = bs_call_price(close, pos['k2'], rem_t, R, IV)
                    pos['status'] = 'spread'
                    next_positions.append(pos)
                else:
                    trades.append(curr_opt_price - pos['premium_paid'])
                continue
        next_positions.append(pos)
    positions = next_positions

    has_active_long = any(p['status'] == 'long_call' for p in positions)
    if not has_active_long and not pd.isna(bb_low):
        is_bottom_signal = (div in ['Bottom', 'BottomSub']) and (uturn == 1)
        if is_bottom_signal and dif < DIF_ENTRY and low < bb_low:
            k1 = close
            entry_premium = bs_call_price(close, k1, T_DAYS / 365.25, R, IV)
            positions.append({
                'status': 'long_call', 'entry_time': curr_time, 'entry_close': close,
                'k1': k1, 'premium_paid': entry_premium, 'expiry': curr_time + timedelta(days=T_DAYS)
            })

# 6. 计算统计结果 (增强版)
pnl_pts = np.array(trades)
pnl_usd = pnl_pts * NQ_VAL

# A. 基础指标
total_pnl = np.sum(pnl_usd)
win_trades = pnl_usd[pnl_usd > 0]
loss_trades = pnl_usd[pnl_usd <= 0]
win_rate = len(win_trades) / len(pnl_usd) if len(pnl_usd) > 0 else 0

# B. 盈亏比 (Profit-Loss Ratio)
avg_win = np.mean(win_trades) if len(win_trades) > 0 else 0
avg_loss = np.abs(np.mean(loss_trades)) if len(loss_trades) > 0 else 0
profit_loss_ratio = avg_win / avg_loss if avg_loss != 0 else float('inf')

# C. 资金曲线与最大回撤 (Max Drawdown)
# 假设初始本金 INIT_CAP，按交易结算顺序计算累计净值
equity_curve = INIT_CAP + np.cumsum(pnl_usd)
# 计算历史最高点
running_max = np.maximum.accumulate(equity_curve)
# 计算回撤金额
drawdowns = running_max - equity_curve
max_drawdown_usd = np.max(drawdowns) if len(drawdowns) > 0 else 0
# 计算回撤百分比
drawdown_pcts = drawdowns / running_max
max_drawdown_pct = np.max(drawdown_pcts) if len(drawdown_pcts) > 0 else 0

# D. 收益风险比 (Return-to-Risk Ratio)
# 通常定义为: 总盈亏 / 最大回撤金额
return_risk_ratio = total_pnl / max_drawdown_usd if max_drawdown_usd != 0 else float('inf')

# 7. 打印完整报告
print(f"--- 策略 2 增强测试报告 ---")
print(f"初始本金: ${INIT_CAP:,.2f}")
print(f"最终净值: ${equity_curve[-1] if len(equity_curve)>0 else INIT_CAP:,.2f}")
print(f"总净盈亏: ${total_pnl:,.2f} ({ (total_pnl/INIT_CAP)*100 :.2f}%)")
print(f"交易次数: {len(trades)}")
print(f"胜率: {win_rate*100:.2f}%")
print(f"盈亏比: {profit_loss_ratio:.2f}")
print(f"平均单笔盈利: {np.mean(pnl_pts) if len(pnl_pts)>0 else 0:.2f} 点")
print(f"最大回撤 (金额): ${max_drawdown_usd:,.2f}")
print(f"最大回撤 (百分比): {max_drawdown_pct*100:.2f}%")
print(f"收益风险比: {return_risk_ratio:.2f}")