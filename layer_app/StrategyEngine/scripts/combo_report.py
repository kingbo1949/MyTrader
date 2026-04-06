"""
combo_report.py — Put+Call Iron Condor 组合策略研究报告生成器

输出文件: output/summary/put_call_combo_report.md

用法:
    cd layer_app/StrategyEngine
    /path/to/quant_env/bin/python3 scripts/combo_report.py

配置:
    TICKERS   — 参与分析的品种列表（按需增减）
    INTERVALS — 参与组合的时间周期列表（当前仅支持 M5 / M15）

方法说明:
    对每个品种测试 2×2 = 4 种 Call/Put 周期搭配。
    - 月度 PnL 相关系数 < 0 → 有对冲价值，选负相关中 Sharpe 最高者
    - 全部正相关 → 选同周期（M5+M5 或 M15+M15）中 Sharpe 最高者
    组合资本基数 = max(平均 put 保证金, 平均 call 保证金)，反映 Iron Condor
    真实资金占用（保证金 = spread_width - net_credit，已扣除权利金收入）。
    最大回撤率 = (equity - peak_equity) / peak_equity，与优化器 stats.py 一致。
"""

import sys
from pathlib import Path

import pandas as pd
import numpy as np
import yaml

# ─── 路径配置 ──────────────────────────────────────────────────────────────────
SCRIPT_DIR  = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent
BASE        = PROJECT_DIR / "output"
OUT_PATH    = BASE / "summary" / "put_call_combo_report.md"

sys.path.insert(0, str(PROJECT_DIR / "src"))

# ─── 用户配置（按需修改）──────────────────────────────────────────────────────
TICKERS   = ["ES", "NQ", "QQQ", "SPY", "IWM", "SMH", "GS", "JPM", "TSM"]
INTERVALS = ["M5", "M15"]   # 当前仅支持 M5 / M15
SINGLE_CAP = 100_000.0      # 单策略保守基准资金（用于第2节单策略表）

TICKER_NOTES = {
    "ES":  "交叉周期负相关，0亏损月，无回撤",
    "NQ":  "全正相关，同周期年化最高",
    "QQQ": "全正相关，同周期 Sharpe 最优",
    "SPY": "全正相关，同周期 0 亏损月",
    "IWM": "全组合均负相关，交叉周期 Sharpe 最优",
    "SMH": "仅 M5+M5 负相关，M15+M5 无亏损月但正相关",
    "GS":  "交叉周期负相关，Sharpe 最高，MaxDD 极小",
    "JPM": "负相关最强，Sharpe 最高，MaxDD=0",
    "TSM": "全正相关，同周期 Sharpe 最优",
}

# ─── 加载合约乘数 ─────────────────────────────────────────────────────────────
def _load_multipliers() -> dict:
    cfg_path = PROJECT_DIR / "config" / "settings.yaml"
    with open(cfg_path, encoding="utf-8") as f:
        settings = yaml.safe_load(f)
    return {k: v.get("multiplier", 1.0) for k, v in settings.get("codeIds", {}).items()}

MULTIPLIERS = _load_multipliers()

# ─── 数据加载 ─────────────────────────────────────────────────────────────────
def _load_csv(ticker: str, strategy: str, interval: str, direction: str) -> pd.DataFrame | None:
    fname = f"UTurnBottomShort{strategy}_{ticker}_{interval}_{direction}.csv"
    path = BASE / ticker / "deallist" / fname
    return pd.read_csv(path) if path.exists() else None


def load_monthly(ticker: str, strategy: str, interval: str, direction: str) -> pd.Series | None:
    df = _load_csv(ticker, strategy, interval, direction)
    if df is None:
        return None
    dt_col = "settlement_dt" if "settlement_dt" in df.columns else "exit_time"
    df["_dt"] = pd.to_datetime(df[dt_col])
    df["month"] = df["_dt"].dt.to_period("M")
    pnl_col = "final_pnl" if "final_pnl" in df.columns else "pnl"
    return df.groupby("month")[pnl_col].sum() * MULTIPLIERS.get(ticker, 1.0)


def avg_margin_dollars(ticker: str, strategy: str, interval: str, direction: str) -> float | None:
    """保证金 = (spread_width - net_credit) × multiplier（已扣除收到的权利金）"""
    df = _load_csv(ticker, strategy, interval, direction)
    if df is None:
        return None
    mult = MULTIPLIERS.get(ticker, 1.0)
    width = (df["k1"] - df["k2"]).abs()
    return float((width - df["net_credit"]).mean() * mult)

# ─── 统计函数 ─────────────────────────────────────────────────────────────────
def _max_dd_from_trades(ticker: str, strategy: str, interval: str,
                        direction: str, cap: float) -> float:
    """逐笔结算计算最大回撤，避免月度聚合丢失月内回撤。"""
    df = _load_csv(ticker, strategy, interval, direction)
    if df is None:
        return 0.0
    dt_col = "settlement_dt" if "settlement_dt" in df.columns else "exit_time"
    pnl_col = "final_pnl" if "final_pnl" in df.columns else "pnl"
    df = df.sort_values(dt_col)
    pnl = df[pnl_col] * MULTIPLIERS.get(ticker, 1.0)
    equity = cap + pnl.cumsum().values
    peak = np.maximum.accumulate(equity)
    return float(((equity - peak) / peak).min() * 100)


def _max_dd_combo(ticker: str, c_iv: str, p_iv: str, cap: float) -> float:
    """组合逐笔回撤：合并两策略的结算记录，按时间排序后计算。"""
    def _load_pnl_series(strat, iv, direction):
        df = _load_csv(ticker, strat, iv, direction)
        if df is None:
            return None
        dt_col = "settlement_dt" if "settlement_dt" in df.columns else "exit_time"
        pnl_col = "final_pnl" if "final_pnl" in df.columns else "pnl"
        s = df[[dt_col, pnl_col]].copy()
        s.columns = ["dt", "pnl"]
        s["pnl"] *= MULTIPLIERS.get(ticker, 1.0)
        return s
    sc = _load_pnl_series("Call", c_iv, "SHORT")
    sp = _load_pnl_series("Put",  p_iv, "LONG")
    if sc is None or sp is None:
        return 0.0
    combined = pd.concat([sc, sp]).sort_values("dt")
    equity = cap + combined["pnl"].cumsum().values
    peak = np.maximum.accumulate(equity)
    return float(((equity - peak) / peak).min() * 100)


def _recovery_factor(total_pnl: float, cap: float, dd_pct: float) -> float:
    """recovery_factor = total_return / abs(max_dd)，与 stats.py 一致"""
    total_return = total_pnl / cap
    abs_dd = abs(dd_pct / 100)
    return total_return / abs_dd if abs_dd > 0 else 0.0


def single_stats(s: pd.Series, cap: float,
                 ticker: str, strategy: str, interval: str, direction: str) -> tuple:
    n = len(s)
    ann_pct = float(s.sum() / n * 12 / cap * 100)
    mu, sd = float(s.mean()), float(s.std())
    sharpe = mu / sd * np.sqrt(12) if sd > 0 else 0.0
    dd_pct = _max_dd_from_trades(ticker, strategy, interval, direction, cap)
    recov = _recovery_factor(float(s.sum()), cap, dd_pct)
    return ann_pct, sharpe, dd_pct, recov, int((s < 0).sum()), n


def combo_stats(s1: pd.Series, s2: pd.Series, cap: float,
                ticker: str, c_iv: str, p_iv: str) -> tuple:
    combined = s1.add(s2, fill_value=0)
    corr = float(s1.corr(s2))
    n = len(combined)
    ann_pct = float(combined.sum() / n * 12 / cap * 100)
    mu, sd = float(combined.mean()), float(combined.std())
    sharpe = mu / sd * np.sqrt(12) if sd > 0 else 0.0
    dd_pct = _max_dd_combo(ticker, c_iv, p_iv, cap)
    recov = _recovery_factor(float(combined.sum()), cap, dd_pct)
    return corr, ann_pct, sharpe, dd_pct, recov, int((combined < 0).sum()), n, combined

# ─── 组合枚举 ─────────────────────────────────────────────────────────────────
def _build_combos(ivs: list[str]) -> list[tuple]:
    """返回 (label, call_iv, put_iv) 所有组合"""
    return [(f"C_{c}+P_{p}", c, p) for c in ivs for p in ivs]


def _recommend(combos: dict) -> str:
    neg = {lbl: v for lbl, v in combos.items() if v["corr"] < 0}
    if neg:
        return max(neg, key=lambda x: neg[x]["sharpe"])
    same = {lbl: v for lbl, v in combos.items()
            if lbl in (f"C_{iv}+P_{iv}" for iv in INTERVALS)}
    pool = same if same else combos
    return max(pool, key=lambda x: pool[x]["sharpe"])

# ─── 主计算 ───────────────────────────────────────────────────────────────────
def build_all_data() -> dict:
    all_data = {}
    for ticker in TICKERS:
        singles, combos = {}, {}
        for iv in INTERVALS:
            for strat, direction in [("Call", "SHORT"), ("Put", "LONG")]:
                s = load_monthly(ticker, strat, iv, direction)
                if s is not None:
                    singles[f"{strat}_{iv}"] = single_stats(
                        s, SINGLE_CAP, ticker, strat, iv, direction)

        for label, c_iv, p_iv in _build_combos(INTERVALS):
            s_call = load_monthly(ticker, "Call", c_iv, "SHORT")
            s_put  = load_monthly(ticker, "Put",  p_iv, "LONG")
            if s_call is None or s_put is None:
                continue
            pm = avg_margin_dollars(ticker, "Put",  p_iv, "LONG")
            cm = avg_margin_dollars(ticker, "Call", c_iv, "SHORT")
            cap = max(pm, cm) if pm and cm else SINGLE_CAP
            corr, ann, sharpe, dd, recov, loss, n, monthly = combo_stats(
                s_call, s_put, cap, ticker, c_iv, p_iv)
            combos[label] = {
                "corr": corr, "ann": ann, "sharpe": sharpe, "dd": dd, "recov": recov,
                "loss": loss, "n": n, "cap": cap,
                "monthly": {str(k): float(v) for k, v in monthly.items()},
            }
        all_data[ticker] = {"singles": singles, "combos": combos}
    return all_data

# ─── 报告生成 ─────────────────────────────────────────────────────────────────
def _a(lines: list, text: str = "") -> None:
    lines.append(text)


def generate_report(all_data: dict) -> str:
    L: list[str] = []

    _a(L, "# Put+Call 组合策略研究报告")
    _a(L)
    _a(L, "**策略：** UTurnBottomShortPut（底部 Mix UTurn → 卖 Bull Put Spread）")
    _a(L, "**+** UTurnBottomShortCall（顶部 Mix UTurn → 卖 Bear Call Spread）")
    _a(L)
    _a(L, "**回测区间：** 2024-01-01 ～ 2026-03-28（约 27 个月）")
    _a(L, f"**研究品种：** {'、'.join(TICKERS)}")
    _a(L, "**合约乘数：** ES=50、NQ=20、股票期权=100（已计入收益率计算）")
    _a(L, "**最大回撤定义：** (equity − peak_equity) / peak_equity，与优化器一致")
    _a(L)
    _a(L, "### 资金假设说明")
    _a(L)
    _a(L, "- **单策略：** 初始资金 $100,000（保守基准，实际保证金远低于此）")
    _a(L, "- **组合（Iron Condor）：** 资本基数 = max(平均 put 保证金, 平均 call 保证金)。")
    _a(L, "  Iron Condor 最大亏损只发生在一侧，券商仅要求较宽一腿的保证金。")
    _a(L, "  保证金 = (spread_width − net_credit) × multiplier，已扣除收到的权利金。")
    _a(L)
    _a(L, "---")
    _a(L)

    # §1 研究方法
    _a(L, "## 1. 研究方法")
    _a(L)
    _a(L, f"对每个品种，测试 {len(INTERVALS)}×{len(INTERVALS)} = {len(INTERVALS)**2} 种区间搭配：")
    _a(L)
    _a(L, "| 组合 | Call 周期 | Put 周期 |")
    _a(L, "|------|-----------|---------|")
    for lbl, c_iv, p_iv in _build_combos(INTERVALS):
        _a(L, f"| {lbl} | {c_iv} | {p_iv} |")
    _a(L)
    _a(L, "**选择原则：**")
    _a(L, "- 月度 PnL 相关系数 < 0 → 有对冲价值，优先选负相关中 Sharpe 最高者")
    _a(L, "- 全部正相关 → 无对冲价值，直接选同周期（M5+M5 或 M15+M15）中表现最优者")
    _a(L)
    _a(L, "**评估指标：** 月度相关系数、年化收益率（%）、Sharpe 比率、最大回撤率（%）、亏损月数")
    _a(L)
    _a(L, "---")
    _a(L)

    # §2 单策略表现
    _a(L, "## 2. 单策略基准表现")
    _a(L)
    _a(L, "*年化收益率基于保守基准 $100,000；最大回撤率基于峰值净值*")
    _a(L)
    _a(L, "| 品种 | 策略 | 年化收益率 | Sharpe | 最大回撤率 | 收益风险比 | 亏损月/总月 |")
    _a(L, "|------|------|-----------|--------|-----------|-----------|------------|")
    for ticker in TICKERS:
        first = True
        for lbl, (ann, sharpe, dd, recov, loss, n) in all_data[ticker]["singles"].items():
            t = ticker if first else ""
            _a(L, f"| {t} | {lbl} | {ann:.1f}% | {sharpe:.2f} | {dd:.2f}% | {recov:.2f} | {loss}/{n} |")
            first = False
        _a(L, "| | | | | | | |")
    _a(L)
    _a(L, "---")
    _a(L)

    # §3 Iron Condor 保证金
    _a(L, "## 3. Iron Condor 实际保证金")
    _a(L)
    _a(L, "*保证金 = (spread_width − net_credit) × multiplier；组合资本基数 = max(put, call)*")
    _a(L)
    _a(L, "| 品种 | 组合 | Put 保证金($) | Call 保证金($) | 组合资本基数($) |")
    _a(L, "|------|------|-------------|--------------|--------------|")
    for ticker in TICKERS:
        first = True
        for lbl, c_iv, p_iv in _build_combos(INTERVALS):
            pm = avg_margin_dollars(ticker, "Put",  p_iv, "LONG")
            cm = avg_margin_dollars(ticker, "Call", c_iv, "SHORT")
            if pm and cm:
                t = ticker if first else ""
                _a(L, f"| {t} | {lbl} | {pm:,.0f} | {cm:,.0f} | {max(pm,cm):,.0f} |")
                first = False
        _a(L, "| | | | | |")
    _a(L)
    _a(L, "---")
    _a(L)

    # §4 2×2 组合对比
    _a(L, "## 4. 2×2 组合对比（Iron Condor 保证金口径）")
    _a(L)
    _a(L, "*年化收益率和最大回撤率均基于实际 Iron Condor 保证金；◀ 表示负相关；**加粗**为推荐组合*")
    _a(L)
    _a(L, "| 品种 | 组合 | 相关系数 | 资本基数($) | 年化收益率 | Sharpe | 最大回撤率 | 收益风险比 | 亏损月/总月 |")
    _a(L, "|------|------|---------|-----------|-----------|--------|-----------|-----------|------------|")
    recommendations = {}
    for ticker in TICKERS:
        combos = all_data[ticker]["combos"]
        rec = _recommend(combos)
        recommendations[ticker] = rec
        first = True
        for lbl, v in combos.items():
            t = ticker if first else ""
            nm = " ◀" if v["corr"] < 0 else ""
            bold = "**" if lbl == rec else ""
            _a(L, f"| {t} | {bold}{lbl}{bold} | {v['corr']:.3f}{nm} | {v['cap']:,.0f} | {v['ann']:.1f}% | {v['sharpe']:.2f} | {v['dd']:.2f}% | {v['recov']:.2f} | {v['loss']}/{v['n']} |")
            first = False
        _a(L, "| | | | | | | | | |")
    _a(L)
    _a(L, "---")
    _a(L)

    # §5 推荐汇总
    _a(L, "## 5. 各品种推荐组合汇总")
    _a(L)
    _a(L, "| 品种 | 推荐组合 | 相关系数 | 资本基数($) | 年化收益率 | Sharpe | 最大回撤率 | 收益风险比 | 亏损月 | 说明 |")
    _a(L, "|------|---------|---------|-----------|-----------|--------|-----------|-----------|--------|------|")
    for ticker in TICKERS:
        rec = recommendations[ticker]
        v = all_data[ticker]["combos"][rec]
        nm = " ◀" if v["corr"] < 0 else ""
        note = TICKER_NOTES.get(ticker, "")
        _a(L, f"| {ticker} | {rec} | {v['corr']:.3f}{nm} | {v['cap']:,.0f} | {v['ann']:.1f}% | {v['sharpe']:.2f} | {v['dd']:.2f}% | {v['recov']:.2f} | {v['loss']}/{v['n']} | {note} |")
    _a(L)
    _a(L, "---")
    _a(L)

    # §6 月度明细
    _a(L, "## 6. 月度明细（推荐组合）")
    _a(L)
    for ticker in TICKERS:
        rec = recommendations[ticker]
        v = all_data[ticker]["combos"][rec]
        cap = v["cap"]
        monthly = v["monthly"]
        _a(L, f"### {ticker} — {rec}（Iron Condor 资本基数 ${cap:,.0f}）")
        _a(L)
        _a(L, "| 月份 | PnL($) | 月收益率 |")
        _a(L, "|------|--------|---------|")
        total = 0.0
        for mo, pnl in sorted(monthly.items()):
            mark = " ⚠" if pnl < 0 else ""
            _a(L, f"| {mo} | {pnl:,.0f}{mark} | {pnl/cap*100:.2f}%{mark} |")
            total += pnl
        n = len(monthly)
        _a(L, f"| **合计** | **{total:,.0f}** | **年化 {total/n*12/cap*100:.1f}%** |")
        _a(L)
    _a(L, "---")
    _a(L)

    # §7 结论
    _a(L, "## 7. 关键结论")
    _a(L)
    _a(L, "1. **Iron Condor 资本效率极高：** 实际保证金远低于 $100,000（ES 约 $23k，股票期权 $1k～$7k），")
    _a(L, "   年化收益率以真实保证金口径计算后大幅提升，充分体现了卖方价差策略的资本优势。")
    _a(L)
    _a(L, "2. **负相关规律：** ES、IWM、GS、JPM 的 Put 与 Call 策略在某些周期组合下呈负相关，")
    _a(L, "   具备对冲价值；NQ、QQQ、SPY、TSM 全部正相关，组合仅起叠加收益作用，无对冲效果。")
    _a(L)
    _a(L, "3. **最优周期搭配：** 在有负相关的品种中，**C_M15+P_M5** 反复出现为最优组合（ES、GS、JPM、IWM），")
    _a(L, "   即 Call 用 M15 周期、Put 用 M5 周期。")
    _a(L)
    _a(L, "4. **Put 策略更稳定：** ShortPut 亏损月普遍少于 ShortCall，")
    _a(L, "   Put_M5 在 GS、JPM 均实现 0 亏损月，是组合的压舱石。")
    _a(L)
    _a(L, "5. **SMH 特殊性：** 在 M5/M15 限制下 Call 表现弱（Sharpe < 2.5）。")
    _a(L, "   若放开周期限制，Call_H1+Put_M5 可达 Sharpe=4.96、0 亏损月（另见全周期分析）。")
    _a(L)
    _a(L, "6. **NQ 正相关较强：** 四种组合相关系数均为正（0.010～0.184），")
    _a(L, "   Put+Call 同向波动，组合风险分散效果有限，需单独监控回撤。")

    return "\n".join(L)


def main():
    print("正在计算各品种组合数据...")
    all_data = build_all_data()
    report = generate_report(all_data)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(report, encoding="utf-8")
    print(f"报告已写入: {OUT_PATH}")
    print(f"品种数: {len(TICKERS)}  组合数/品种: {len(INTERVALS)**2}")


if __name__ == "__main__":
    main()
