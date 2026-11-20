"""
IBKR 个人基金份额账本维护脚本。

用 CSV（非 xlsx）——纯文本格式，CLion/PyCharm 都能直接内置打开/以表格形式查看，
不需要 Numbers/Excel 这类外部程序。

记账规则（已与用户确认）：
  - valuation（估值更新）：告知某天账户总额（CAD），不改变份额总数，
    重新算出 每份净值 = 新总额 / 份额总数。
  - deposit / withdrawal（入金/出金）：份额总数按"这笔现金流 / 入金前最新
    每份净值"换算成份额变化，每份净值本身在这笔操作前后保持不变（出入金
    不应该被误算成盈亏）。账户总额 = 操作前总额 + 现金流（正负号已含符号）。
  - 第一行必须是 deposit（建仓）：每份净值定义为 1.00，份额总数 = 入金金额。

本脚本是"记一笔、算一遍"的命令行工具，不是持续运行的程序——每次用户在对话里
报告新数据，就调用一次，脚本读现有 ibkr_fund_ledger.csv、追加一行、重算
ibkr_fund_summary.csv，保证每次用的都是同一套计算逻辑，不依赖手工改表格。

用法：
  python3 fund_ledger.py --date 2026-08-26 --type deposit    --amount 131946
  python3 fund_ledger.py --date 2026-09-01 --type valuation  --amount 133500
  python3 fund_ledger.py --date 2026-09-15 --type withdrawal --amount 5000 --note "备注"
"""
import argparse
import csv
from datetime import date, datetime
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.lines import Line2D
from matplotlib.transforms import blended_transform_factory

matplotlib.rcParams["font.family"] = "PingFang SC"  # macOS 自带中文字体，避免中文标签显示为方块
matplotlib.rcParams["axes.unicode_minus"] = False

_DIR = Path(__file__).resolve().parent
LEDGER_CSV  = _DIR / "ibkr_fund_ledger.csv"
SUMMARY_CSV = _DIR / "ibkr_fund_summary.csv"
NAV_CHART_PNG = _DIR / "ibkr_fund_nav.png"

LEDGER_HEADERS = [
    "日期", "事件类型", "现金流(CAD)", "份额变化", "份额总数", "每份净值(CAD)", "账户总额(CAD)", "备注",
]
EVENT_LABELS = {"deposit": "入金", "withdrawal": "出金", "valuation": "估值更新"}


def _read_ledger_rows() -> list[dict]:
    if not LEDGER_CSV.exists():
        return []
    with open(LEDGER_CSV, newline='', encoding='utf-8') as f:
        return list(csv.DictReader(f))


def _last_state(rows: list[dict]) -> tuple[float, float, float]:
    """返回 (份额总数, 每份净值, 账户总额) 的最新状态，账本为空时返回 (0,0,0)。"""
    if not rows:
        return 0.0, 0.0, 0.0
    last = rows[-1]
    return float(last["份额总数"]), float(last["每份净值(CAD)"]), float(last["账户总额(CAD)"])


def _compute_event(rows: list[dict], event_type: str, amount: float) -> dict:
    shares, nav, total_value = _last_state(rows)
    is_first = not rows

    if event_type == "valuation":
        if is_first:
            raise ValueError("账本为空，第一笔必须是 deposit（建仓），不能是 valuation")
        cash_flow    = ""
        shares_delta = 0.0
        new_shares   = shares
        new_total    = amount
        new_nav      = new_total / new_shares if new_shares else 0.0
    else:
        signed = amount if event_type == "deposit" else -amount
        if is_first:
            if event_type != "deposit":
                raise ValueError("账本为空，第一笔必须是 deposit（建仓）")
            new_nav      = 1.0
            shares_delta = amount / new_nav
            new_shares   = shares_delta
            new_total    = amount
        else:
            new_nav      = nav   # 出入金不改变每份净值
            shares_delta = signed / new_nav
            new_shares   = shares + shares_delta
            new_total    = total_value + signed
        cash_flow = round(signed, 2)

    return {
        "cash_flow": cash_flow, "shares_delta": round(shares_delta, 6),
        "shares": round(new_shares, 6), "nav": round(new_nav, 6),
        "total": round(new_total, 2),
    }


def _append_ledger_row(event_date: date, event_type: str, state: dict, note: str) -> None:
    is_new = not LEDGER_CSV.exists()
    with open(LEDGER_CSV, "a", newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        if is_new:
            writer.writerow(LEDGER_HEADERS)
        writer.writerow([
            event_date.isoformat(), EVENT_LABELS[event_type], state["cash_flow"],
            state["shares_delta"], state["shares"], state["nav"], state["total"], note or "",
        ])


def _write_summary(state: dict, event_date: date) -> None:
    with open(SUMMARY_CSV, "w", newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(["项目", "数值"])
        writer.writerow(["份额总数", state["shares"]])
        writer.writerow(["每份净值(CAD)", state["nav"]])
        writer.writerow(["账户总额(CAD)", state["total"]])
        writer.writerow(["最近更新日期", event_date.isoformat()])


def _write_nav_chart() -> None:
    """用账本全部历史行重画净值/总额曲线图：上下两个子图共享横轴（日期），
    上图画每份净值(CAD)，下图画账户总额(CAD)——两者量纲差太大（1.x vs 十万+），
    画在同一坐标系会把净值曲线压成直线，所以分成上下两个独立Y轴的子图。"""
    rows = _read_ledger_rows()
    if not rows:
        return
    dates  = [datetime.strptime(r["日期"], "%Y-%m-%d") for r in rows]
    navs   = [float(r["每份净值(CAD)"]) for r in rows]
    totals = [float(r["账户总额(CAD)"]) for r in rows]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
    fig.suptitle("IBKR 基金净值 / 账户总额曲线", fontsize=14, fontweight="bold")

    ax1.plot(dates, navs, marker="o", markersize=3, color="#2c3e50", lw=1.5)
    ax1.set_ylabel("每份净值 (CAD)")
    ax1.grid(True, alpha=0.3)

    ax2.plot(dates, totals, marker="s", markersize=3, color="#c0392b", lw=1.5)
    ax2.set_ylabel("账户总额 (CAD)")
    ax2.set_xlabel("日期")
    ax2.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d"))
    ax2.grid(True, alpha=0.3)

    # 出入金事件标注：下图画竖直虚线 + 现金流金额，入金绿色/出金橙色
    trans = blended_transform_factory(ax2.transData, ax2.transAxes)
    has_deposit = has_withdrawal = False
    for r in rows:
        if r["事件类型"] not in ("入金", "出金"):
            continue
        dt_num = mdates.date2num(datetime.strptime(r["日期"], "%Y-%m-%d"))
        cash_flow = float(r["现金流(CAD)"])
        is_deposit = r["事件类型"] == "入金"
        color = "#27ae60" if is_deposit else "#e67e22"
        has_deposit |= is_deposit
        has_withdrawal |= not is_deposit
        ax2.axvline(dt_num, color=color, linestyle="--", alpha=0.6, lw=1)
        ax2.annotate(f"{cash_flow:+,.0f}", xy=(dt_num, 1.0), xycoords=trans,
                     xytext=(2, -2), textcoords="offset points",
                     ha="left", va="top", fontsize=8, color=color, rotation=90)
    legend_handles = [Line2D([0], [0], color="#c0392b", marker="s", markersize=4, lw=1.5, label="账户总额")]
    if has_deposit:
        legend_handles.append(Line2D([0], [0], color="#27ae60", linestyle="--", label="入金"))
    if has_withdrawal:
        legend_handles.append(Line2D([0], [0], color="#e67e22", linestyle="--", label="出金"))
    ax2.legend(handles=legend_handles, loc="lower right", fontsize=8)

    fig.autofmt_xdate()
    fig.tight_layout()
    fig.savefig(NAV_CHART_PNG, dpi=150)
    plt.close(fig)


def record_event(event_date: date, event_type: str, amount: float, note: str = "") -> dict:
    rows = _read_ledger_rows()
    state = _compute_event(rows, event_type, amount)
    _append_ledger_row(event_date, event_type, state, note)
    _write_summary(state, event_date)
    _write_nav_chart()
    return state


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--date", required=True, help="YYYY-MM-DD")
    parser.add_argument("--type", required=True, choices=["deposit", "withdrawal", "valuation"])
    parser.add_argument("--amount", required=True, type=float, help="CAD，正数（deposit/withdrawal 的方向由 --type 决定，不用带负号）")
    parser.add_argument("--note", default="", help="可选备注")
    args = parser.parse_args()

    event_date = datetime.strptime(args.date, "%Y-%m-%d").date()
    state = record_event(event_date, args.type, args.amount, args.note)

    print(f"已记录: {args.date} {EVENT_LABELS[args.type]} {args.amount:,.2f} CAD")
    print(f"  份额总数     : {state['shares']:,.6f}")
    print(f"  每份净值(CAD): {state['nav']:,.4f}")
    print(f"  账户总额(CAD): {state['total']:,.2f}")
    print(f"  净值曲线图    : {NAV_CHART_PNG.name}")


if __name__ == "__main__":
    main()