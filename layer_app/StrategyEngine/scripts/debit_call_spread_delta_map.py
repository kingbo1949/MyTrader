"""
debit_call_spread_delta_map.py — Debit Call Spread 到期保本 Delta 映射分析。

给定 Long 腿 Delta，计算标的价格不变时到期恰好保本所需的 Short 腿 Delta。
  期货品种（ES/NQ/GC）自动使用 Black-76，其余（QQQ/SPY 等）使用 Black-Scholes。

运行方式：
    cd layer_app/StrategyEngine
    python3 scripts/debit_call_spread_delta_map.py
    python3 scripts/debit_call_spread_delta_map.py --code ES --price 5800 --iv 0.18
    python3 scripts/debit_call_spread_delta_map.py --code NQ --multi-dte 3 5 7 14
    python3 scripts/debit_call_spread_delta_map.py --code NQ --multi-iv 0.15 0.20 0.25 0.30
    python3 scripts/debit_call_spread_delta_map.py --code QQQ --price 500 --iv 0.25 --no-save
"""
import argparse
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Final

import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import norm

# ── 中文字体 ──────────────────────────────────────────────────────────────────
plt.rcParams['font.sans-serif'] = ['Arial Unicode MS', 'SimHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

_OUTPUT_ROOT  = Path(__file__).resolve().parents[1] / "output"
_FUTURES_CODES: Final[frozenset[str]] = frozenset({'ES', 'NQ', 'GC'})
_DEFAULT_PRICES: Final[dict[str, float]] = {'NQ': 20000.0, 'ES': 5800.0, 'GC': 2800.0}
_KEY_LONG_DELTAS: Final[list[float]] = [0.60, 0.65, 0.70, 0.75, 0.80, 0.85, 0.90]
_KEY_ANNOTATE: Final[list[tuple[float, str]]] = [(0.80, '#ff7f0e'), (0.70, '#2ca02c')]
_COLORS: Final[list[str]] = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd']


# ── 市场状态 ──────────────────────────────────────────────────────────────────

@dataclass(slots=True, frozen=True)
class MarketState:
    """封装一次定价所需的全部市场参数"""
    code:  str
    price: float   # 标的现价 / 期货价格
    iv:    float   # 隐含波动率
    dte:   float   # 到期天数
    r:     float   # 无风险利率

    @property
    def t(self) -> float:
        return self.dte / 365.0

    @property
    def is_futures(self) -> bool:
        return self.code.upper() in _FUTURES_CODES


# ── 定价引擎（BS / B76 双路由）────────────────────────────────────────────────

def _call_price_bs(s: float, k: float, t: float, r: float, iv: float) -> float:
    """Black-Scholes 欧式 Call 定价"""
    if t <= 0:
        return max(0.0, s - k)
    d1 = (math.log(s / k) + (r + 0.5 * iv**2) * t) / (iv * math.sqrt(t))
    d2 = d1 - iv * math.sqrt(t)
    return s * norm.cdf(d1) - k * math.exp(-r * t) * norm.cdf(d2)


def _call_price_b76(f: float, k: float, t: float, r: float, iv: float) -> float:
    """Black-76 期货期权 Call 定价（无漂移项，F 折现）"""
    if t <= 0:
        return max(0.0, f - k)
    d1 = (math.log(f / k) + 0.5 * iv**2 * t) / (iv * math.sqrt(t))
    d2 = d1 - iv * math.sqrt(t)
    return math.exp(-r * t) * (f * norm.cdf(d1) - k * norm.cdf(d2))


def call_price(state: MarketState, k: float) -> float:
    """按品种自动路由 BS / B76"""
    fn = _call_price_b76 if state.is_futures else _call_price_bs
    return fn(state.price, k, state.t, state.r, state.iv)


def _delta_to_strike_b76(f: float, t: float, delta: float, r: float, iv: float) -> float:
    """B76：delta = e^{-rT}·N(d1) → 调整 delta 后反推 K"""
    adjusted = min(delta * math.exp(r * max(t, 0.0)), 1.0 - 1e-9)
    d1 = norm.ppf(max(adjusted, 1e-9))
    return f * math.exp(-d1 * iv * math.sqrt(max(t, 1e-8)))


def _delta_to_strike_bs(s: float, t: float, delta: float, r: float, iv: float) -> float:
    """BS：delta = N(d1) → 含完整漂移项的精确反推"""
    d1 = norm.ppf(delta)
    return s * math.exp(-d1 * iv * math.sqrt(t) + (r + 0.5 * iv**2) * t)


def delta_to_strike(state: MarketState, delta: float) -> float:
    fn = _delta_to_strike_b76 if state.is_futures else _delta_to_strike_bs
    return fn(state.price, state.t, delta, state.r, state.iv)


def _strike_to_delta_b76(f: float, k: float, t: float, r: float, iv: float) -> float:
    if t <= 0:
        return 1.0 if f > k else 0.0
    d1 = (math.log(f / k) + 0.5 * iv**2 * t) / (iv * math.sqrt(t))
    return math.exp(-r * t) * norm.cdf(d1)


def _strike_to_delta_bs(s: float, k: float, t: float, r: float, iv: float) -> float:
    if t <= 0:
        return 1.0 if s > k else 0.0
    d1 = (math.log(s / k) + (r + 0.5 * iv**2) * t) / (iv * math.sqrt(t))
    return float(norm.cdf(d1))


def strike_to_delta(state: MarketState, k: float) -> float:
    fn = _strike_to_delta_b76 if state.is_futures else _strike_to_delta_bs
    return fn(state.price, k, state.t, state.r, state.iv)


# ── 保本映射核心 ──────────────────────────────────────────────────────────────

def find_breakeven_short_delta(state: MarketState, long_delta: float) -> float | None:
    """
    计算：当标的价格到期不变时，使 Debit Call Spread 恰好保本的 Short 腿 Delta。

    保本公式推导：
        到期 P&L = Long内在值 − (Long权利金 − Short权利金)
                 = Short权利金 − Long时间价值
        保本 → Short权利金 = Long时间价值
    """
    k_long      = delta_to_strike(state, long_delta)
    long_prem   = call_price(state, k_long)
    long_intrinsic = max(0.0, state.price - k_long)
    target_short_prem = long_prem - long_intrinsic  # Long 腿时间价值

    if target_short_prem < 0:
        return None

    # 在 ATM → +15% 区间数值搜索
    strikes  = np.linspace(state.price, state.price * 1.15, 3000)
    premiums = np.array([call_price(state, float(k)) for k in strikes])
    idx      = int(np.argmin(np.abs(premiums - target_short_prem)))

    if np.abs(premiums[idx] - target_short_prem) > 1.0:
        return None  # 残差过大：期权链无法覆盖目标权利金

    return strike_to_delta(state, float(strikes[idx]))


def compute_curve(state: MarketState, long_deltas: np.ndarray) -> tuple[list, list]:
    """计算一条映射曲线，过滤掉计算失败的点"""
    valid_long, valid_short = [], []
    for ld in long_deltas:
        sd = find_breakeven_short_delta(state, float(ld))
        if sd is not None:
            valid_long.append(ld)
            valid_short.append(sd)
    return valid_long, valid_short


# ── 汇总表格 ──────────────────────────────────────────────────────────────────

def _print_summary_table(states: list[MarketState]) -> None:
    """打印各场景下的保本 Short Delta 查询表"""
    labels = [f"{s.dte:.0f}DTE/IV{s.iv*100:.0f}%" for s in states]
    col_w  = max(len(lb) for lb in labels) + 2
    header = f"{'Long Δ':>8}" + "".join(f"  {lb:>{col_w}}" for lb in labels)
    sep    = "─" * len(header)
    print(f"\n── Debit Call Spread 到期保本 Short Delta 汇总 {'─'*20}")
    print(f"   {'✓ 保本线以上的 Short Δ = 标的不动也不亏；以下 = 需要上涨才盈利':}")
    print(header)
    print(sep)
    for ld in _KEY_LONG_DELTAS:
        row = f"{ld:>8.2f}"
        for state in states:
            sd  = find_breakeven_short_delta(state, ld)
            val = f"{sd:.3f}" if sd is not None else "N/A"
            row += f"  {val:>{col_w}}"
        print(row)
    print()


# ── 绘图 ──────────────────────────────────────────────────────────────────────

def _annotate_key_points(ax, state: MarketState) -> None:
    """标注 Long=0.70 和 Long=0.80 两个典型工作点"""
    for ld, color in _KEY_ANNOTATE:
        sd = find_breakeven_short_delta(state, ld)
        if sd is None:
            continue
        ax.scatter(ld, sd, color=color, s=70, zorder=6)
        ax.annotate(f"Δ{ld:.2f}→{sd:.2f}",
                    xy=(ld, sd),
                    xytext=(ld + 0.025, sd - 0.045),
                    fontsize=8.5,
                    color=color,
                    arrowprops=dict(arrowstyle="->", color=color, lw=1.1))


def _build_title(base: MarketState, mode: str) -> str:
    model = "Black-76（期货）" if base.is_futures else "Black-Scholes（现货）"
    group = "DTE" if mode == 'dte' else "IV"
    return (f"Debit Call Spread 到期保本映射曲线\n"
            f"{base.code} | {model} | S={int(base.price)} | 按 {group} 分组")


def _plot_curves(states: list[MarketState], mode: str, save_path: Path | None) -> None:
    long_deltas = np.linspace(0.52, 0.95, 80)
    fig, ax = plt.subplots(figsize=(11, 6.5), dpi=120)

    for i, state in enumerate(states):
        xs, ys = compute_curve(state, long_deltas)
        label  = f"{state.dte:.0f}DTE" if mode == 'dte' else f"IV={state.iv*100:.0f}%"
        ax.plot(xs, ys, label=label, color=_COLORS[i % len(_COLORS)], linewidth=2.2)
        if i == 0:
            _annotate_key_points(ax, state)

    # 参考线：Short Delta = Long Delta（对角线，表示两腿等 Delta，无意义区）
    ax.plot([0.50, 0.65], [0.50, 0.65], linestyle=':', color='gray', linewidth=1.0,
            label="Short=Long（无效区）")

    ax.set_title(_build_title(states[0], mode), fontsize=12, pad=12)
    ax.set_xlabel("Long 腿开仓 Delta（实值方向，买入）", fontsize=10)
    ax.set_ylabel("Short 腿保本 Delta（虚值方向，卖出）", fontsize=10)
    ax.set_xlim(0.50, 0.98)
    ax.set_ylim(0.0, 0.65)
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.legend(loc="upper right", fontsize=9)
    fig.tight_layout()

    if save_path:
        save_path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(save_path, dpi=150)
        print(f"图表已保存：{save_path}")

    plt.show()


# ── CLI 解析 ──────────────────────────────────────────────────────────────────

def _parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Debit Call Spread 到期保本 Delta 映射（支持 BS / B76 自动路由）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""示例：
  python3 scripts/debit_call_spread_delta_map.py
  python3 scripts/debit_call_spread_delta_map.py --code ES --price 5800
  python3 scripts/debit_call_spread_delta_map.py --code NQ --multi-dte 3 5 7 14
  python3 scripts/debit_call_spread_delta_map.py --code QQQ --price 500 --multi-iv 0.15 0.20 0.25 0.30
""")
    p.add_argument("--code",  default="NQ",
                   help="品种代码（ES/NQ/GC 用 Black-76，其余用 Black-Scholes），默认 NQ")
    p.add_argument("--price", type=float, default=None,
                   help="标的现价（不填则按内置默认：NQ=20000, ES=5800, GC=2800, 其余=500）")
    p.add_argument("--iv",    type=float, default=0.20,
                   help="隐含波动率（0~1），默认 0.20")
    p.add_argument("--dte",   type=float, default=7.0,
                   help="到期天数，默认 7")
    p.add_argument("--r",     type=float, default=0.045,
                   help="无风险利率（0~1），默认 0.045")
    p.add_argument("--multi-dte", type=float, nargs="+", default=None, metavar="D",
                   help="叠加多条 DTE 曲线，例：--multi-dte 3 5 7 14")
    p.add_argument("--multi-iv",  type=float, nargs="+", default=None, metavar="V",
                   help="叠加多条 IV 曲线，例：--multi-iv 0.15 0.20 0.25 0.30")
    p.add_argument("--no-save", action="store_true",
                   help="不将图表保存到 output/{CODE}/charts/ 目录")
    return p.parse_args()


def _build_states(args: argparse.Namespace) -> tuple[list[MarketState], str]:
    code  = args.code.upper()
    price = args.price if args.price is not None else _DEFAULT_PRICES.get(code, 500.0)

    if args.multi_iv:
        return [MarketState(code, price, iv, args.dte, args.r) for iv in args.multi_iv], 'iv'
    if args.multi_dte:
        return [MarketState(code, price, args.iv, dte, args.r) for dte in args.multi_dte], 'dte'
    return [MarketState(code, price, args.iv, args.dte, args.r)], 'dte'


def _resolve_save_path(base: MarketState, states: list[MarketState], mode: str,
                        no_save: bool) -> Path | None:
    if no_save:
        return None
    tag = ('dte' + '_'.join(f"{int(s.dte)}" for s in states) if mode == 'dte'
           else 'iv' + '_'.join(f"{int(s.iv * 100)}" for s in states))
    return _OUTPUT_ROOT / base.code / "charts" / f"debit_call_breakeven_{base.code}_{tag}.png"


def main() -> None:
    args          = _parse_args()
    states, mode  = _build_states(args)
    base          = states[0]
    model         = "Black-76（期货）" if base.is_futures else "Black-Scholes（现货）"
    save_path     = _resolve_save_path(base, states, mode, args.no_save)

    print(f"\n{'='*60}")
    print(f"  品种: {base.code}  |  定价模型: {model}")
    print(f"  现价: {base.price}  |  IV: {base.iv*100:.1f}%  |  "
          f"DTE: {base.dte}  |  r: {base.r*100:.1f}%")
    print(f"{'='*60}")

    _print_summary_table(states)
    _plot_curves(states, mode, save_path)


if __name__ == "__main__":
    main()
