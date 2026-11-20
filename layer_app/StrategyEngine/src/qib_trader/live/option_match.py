"""
真实期权链匹配：纯函数，不 import ib_insync，可脱离 IB 连接单测。

设计要点（详见实盘信号系统设计文档）：不用真实市场报价反推真实 delta，只用
真实链的"到期日/行权价"元数据做两次吸附——
  1. 从真实到期日列表里选最接近策略目标 DTE 的那个；
  2. 用这个真实 DTE 代入现有的 option_pricing 模块重新算一次理论行权价
     （target_delta 为 None 时，说明这条腿是复用一个已知的固定行权价，跳过这步）；
  3. 把行权价吸附到真实链上最接近的行权价；
  4. 用最终这组"真实到期日+真实行权价"算出理论权利金。
不依赖账户的期权行情分析权限（modelGreeks/OPRA entitlement），只需要最基础的
合约元数据查询权限（ib.reqSecDefOptParams）。
"""
from dataclasses import dataclass
from datetime import datetime

from qib_trader.utils import option_pricing


@dataclass
class OptionChainMeta:
    """从 ib_insync 的 OptionChain 转换而来的最小元数据（避免 option_match.py 依赖 ib_insync 类型）。"""
    expirations: list   # ['20260814', ...] YYYYMMDD 字符串
    strikes: list        # [450.0, 452.5, ...]


@dataclass
class MatchResult:
    expiry: str           # 真实到期日 YYYYMMDD
    strike: float          # 真实行权价
    theo_price: float       # 用真实DTE（+目标delta或给定行权价）重算出的理论权利金
    dte_gap: int            # 真实DTE - 目标DTE（正数=比目标晚到期）
    strike_gap: float       # 真实行权价 - BS重算的理论行权价（给定行权价场景下为0）


def days_to(expiry_str: str, from_dt: datetime) -> int:
    expiry_date = datetime.strptime(expiry_str, '%Y%m%d').date()
    return (expiry_date - from_dt.date()).days


def match_real_contract(chain: OptionChainMeta, right: str, S: float, r: float, iv: float,
                        code_id: str, strike_round: float, now_dt: datetime, *,
                        target_delta: float = None, target_days: int = None,
                        given_strike: float = None) -> MatchResult | None:
    """
    right: 'call'/'put'。
    target_delta+target_days：常规场景，按目标delta在真实DTE下重新反推行权价。
    given_strike（target_delta 为 None 时使用）：已知固定行权价的场景（比如价差态
    下跌回调腿，复用原有 K2，不需要重新按 delta 反推），只做到期日+行权价的双重吸附。
    返回 None 表示链数据不足（没有到期日/行权价列表）或参数不足，调用方应该跳过
    这次匹配、只展示理论值、不展示"真实合约"部分。
    """
    if not chain.expirations or not chain.strikes or target_days is None:
        return None

    real_expiry = min(chain.expirations, key=lambda e: abs(days_to(e, now_dt) - target_days))
    real_dte = days_to(real_expiry, now_dt)
    T = max(real_dte, 0) / 365.25

    if target_delta is not None:
        theo_k = (option_pricing.call_strike_by_delta(S, T, target_delta, iv, r, code_id, strike_round)
                  if right == 'call' else
                  option_pricing.put_strike_by_delta(S, T, target_delta, iv, r, code_id, strike_round))
    else:
        theo_k = given_strike
    if theo_k is None:
        return None

    real_k = min(chain.strikes, key=lambda k: abs(k - theo_k))
    theo_price = (option_pricing.call_price(S, real_k, T, r, iv, code_id) if right == 'call'
                  else option_pricing.put_price(S, real_k, T, r, iv, code_id))

    return MatchResult(
        expiry=real_expiry, strike=real_k, theo_price=theo_price,
        dte_gap=real_dte - target_days, strike_gap=real_k - theo_k,
    )
