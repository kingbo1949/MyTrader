"""
期权定价统一模块。

路由规则：
  FUTURES_CODES（ES / NQ / GC）→ Black-76（无持有成本项，F 折现）
  其余品种（ETF、个股）        → Black-Scholes

核心差异：
  Black-Scholes  d1 = [ln(S/K) + (r+0.5σ²)T] / (σ√T)
                 Call = S·N(d1) − K·e^{-rT}·N(d2)
                 Call Δ = N(d1)

  Black-76       d1 = [ln(F/K) + 0.5σ²T] / (σ√T)   ← 无 r·T 漂移项
                 Call = e^{-rT}·[F·N(d1) − K·N(d2)]  ← F 折现
                 Call Δ = e^{-rT}·N(d1)

公开 API（按 code_id 自动路由）：
  call_price(F_or_S, K, T, r, sigma, code_id)
  put_price(F_or_S, K, T, r, sigma, code_id)
  call_strike_by_delta(F_or_S, T, delta, iv, r, code_id, strike_round)
  put_strike_by_delta(F_or_S, T, delta_abs, iv, r, code_id, strike_round)
  call_delta(F_or_S, K, T, r, sigma, code_id)
  put_delta_abs(F_or_S, K, T, r, sigma, code_id)
"""
import math
from scipy.stats import norm

# ── 期货合约集合（使用 Black-76）─────────────────────────────────────────────
FUTURES_CODES: frozenset[str] = frozenset({'ES', 'NQ', 'GC'})


def is_futures(code_id: str) -> bool:
    """判断是否为需要 Black-76 的期货合约。"""
    return code_id in FUTURES_CODES


# ── 内部：Black-Scholes（现货 ETF / 个股）─────────────────────────────────────

def _bs_d1(S: float, K: float, T: float, r: float, sigma: float) -> float:
    return (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))


def _bs_call(S: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return max(S - K, 0.0)
    d1 = _bs_d1(S, K, T, r, sigma)
    d2 = d1 - sigma * math.sqrt(T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


def _bs_put(S: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return max(K - S, 0.0)
    d1 = _bs_d1(S, K, T, r, sigma)
    d2 = d1 - sigma * math.sqrt(T)
    return K * math.exp(-r * T) * norm.cdf(-d2) - S * norm.cdf(-d1)


def _bs_call_delta(S: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return 1.0 if S > K else 0.0
    return norm.cdf(_bs_d1(S, K, T, r, sigma))


def _bs_put_delta_abs(S: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return 1.0 if K > S else 0.0
    return norm.cdf(-_bs_d1(S, K, T, r, sigma))


def _bs_call_strike(S: float, T: float, delta: float, iv: float,
                    strike_round: float) -> float:
    """BS call Δ = N(d1)；简化反推（忽略漂移项，7-14 DTE 误差 < 0.1%）。"""
    d1 = norm.ppf(max(delta, 1e-9))
    K = S * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
    return round(K / strike_round) * strike_round


def _bs_put_strike(S: float, T: float, delta_abs: float, iv: float,
                   strike_round: float) -> float:
    """BS |put Δ| = N(-d1)；简化反推。"""
    d1 = norm.ppf(max(1.0 - delta_abs, 1e-9))
    K = S * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
    return round(K / strike_round) * strike_round


# ── 内部：Black-76（期货期权）─────────────────────────────────────────────────

def _b76_d1(F: float, K: float, T: float, sigma: float) -> float:
    """Black-76 d1：无 r·T 漂移项（期货价格已是远期价格）。"""
    return (math.log(F / K) + 0.5 * sigma ** 2 * T) / (sigma * math.sqrt(T))


def _b76_call(F: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return max(F - K, 0.0)
    d1 = _b76_d1(F, K, T, sigma)
    d2 = d1 - sigma * math.sqrt(T)
    return math.exp(-r * T) * (F * norm.cdf(d1) - K * norm.cdf(d2))


def _b76_put(F: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return max(K - F, 0.0)
    d1 = _b76_d1(F, K, T, sigma)
    d2 = d1 - sigma * math.sqrt(T)
    return math.exp(-r * T) * (K * norm.cdf(-d2) - F * norm.cdf(-d1))


def _b76_call_delta(F: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return 1.0 if F > K else 0.0
    return math.exp(-r * T) * norm.cdf(_b76_d1(F, K, T, sigma))


def _b76_put_delta_abs(F: float, K: float, T: float, r: float, sigma: float) -> float:
    if T <= 0:
        return 1.0 if K > F else 0.0
    return math.exp(-r * T) * norm.cdf(-_b76_d1(F, K, T, sigma))


def _b76_call_strike(F: float, T: float, delta: float, iv: float,
                     r: float, strike_round: float) -> float:
    """B76 call Δ = e^{-rT}·N(d1)；先调整 delta 再反推行权价。"""
    adjusted = min(delta * math.exp(r * max(T, 0.0)), 1.0 - 1e-9)
    d1 = norm.ppf(max(adjusted, 1e-9))
    K = F * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
    return round(K / strike_round) * strike_round


def _b76_put_strike(F: float, T: float, delta_abs: float, iv: float,
                    r: float, strike_round: float) -> float:
    """B76 |put Δ| = e^{-rT}·N(-d1)；先调整 delta 再反推行权价。"""
    adjusted = min(delta_abs * math.exp(r * max(T, 0.0)), 1.0 - 1e-9)
    d1 = norm.ppf(max(1.0 - adjusted, 1e-9))
    K = F * math.exp(-d1 * iv * math.sqrt(max(T, 1e-8)))
    return round(K / strike_round) * strike_round


# ── 公开 API（按 code_id 自动路由）──────────────────────────────────────────

def call_price(F_or_S: float, K: float, T: float, r: float,
               sigma: float, code_id: str) -> float:
    """欧式 Call 定价：期货用 Black-76，现货用 Black-Scholes。"""
    if K <= 0 or sigma <= 0:
        return max(F_or_S - K, 0.0)
    return _b76_call(F_or_S, K, T, r, sigma) if is_futures(code_id) \
        else _bs_call(F_or_S, K, T, r, sigma)


def put_price(F_or_S: float, K: float, T: float, r: float,
              sigma: float, code_id: str) -> float:
    """欧式 Put 定价：期货用 Black-76，现货用 Black-Scholes。"""
    if K <= 0 or sigma <= 0:
        return max(K - F_or_S, 0.0)
    return _b76_put(F_or_S, K, T, r, sigma) if is_futures(code_id) \
        else _bs_put(F_or_S, K, T, r, sigma)


def call_strike_by_delta(F_or_S: float, T: float, delta: float, iv: float,
                          r: float, code_id: str, strike_round: float) -> float:
    """由 call delta 反推行权价并取整。"""
    return _b76_call_strike(F_or_S, T, delta, iv, r, strike_round) \
        if is_futures(code_id) \
        else _bs_call_strike(F_or_S, T, delta, iv, strike_round)


def put_strike_by_delta(F_or_S: float, T: float, delta_abs: float, iv: float,
                         r: float, code_id: str, strike_round: float) -> float:
    """由 |put delta| 反推行权价并取整。"""
    return _b76_put_strike(F_or_S, T, delta_abs, iv, r, strike_round) \
        if is_futures(code_id) \
        else _bs_put_strike(F_or_S, T, delta_abs, iv, strike_round)


def call_delta(F_or_S: float, K: float, T: float, r: float,
               sigma: float, code_id: str) -> float:
    """计算 Call Delta（供滚仓判断使用）。"""
    return _b76_call_delta(F_or_S, K, T, r, sigma) if is_futures(code_id) \
        else _bs_call_delta(F_or_S, K, T, r, sigma)


def put_delta_abs(F_or_S: float, K: float, T: float, r: float,
                  sigma: float, code_id: str) -> float:
    """计算 |Put Delta|（供滚仓判断使用）。"""
    return _b76_put_delta_abs(F_or_S, K, T, r, sigma) if is_futures(code_id) \
        else _bs_put_delta_abs(F_or_S, K, T, r, sigma)
