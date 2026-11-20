"""
lead_lag_analysis.py — 分析 2~3 个品种 bottom/top 信号的同步性与胜率提升效果。

逻辑：以 ids[0] 信号为基准，在 ±window_mins 分钟内检查其余品种是否同步触发，
      "is_synced" 要求所有品种均在窗口内命中，"exact_sync" 要求所有 diff_mins == 0。

运行方式：
    cd layer_app/StrategyEngine
    python3 scripts/lead_lag_analysis.py
"""
import math
from dataclasses import dataclass
from pathlib import Path

import pandas as pd

_OUTPUT_ROOT = Path(__file__).resolve().parents[1] / "output"


# ── 数据加载 ──────────────────────────────────────────────────────────────────

def _filter_rvold(df: pd.DataFrame, min_rvold: float) -> pd.DataFrame:
    if "rvold" not in df.columns:
        return df
    return df[df["rvold"] >= min_rvold].reset_index(drop=True)


def _load(code_id: str, interval: str, signal_type: str) -> pd.DataFrame:
    path = (_OUTPUT_ROOT / code_id / "signal_test"
            / f"simple_uturn_signal_test_{code_id}_{interval}_{signal_type}.csv")
    if not path.exists():
        raise FileNotFoundError(f"找不到: {path}\n先运行 simple_uturn_signal_test.py 生成数据。")
    df = pd.read_csv(path)
    df["datetime"] = pd.to_datetime(df["datetime"])
    return df.reset_index(drop=True)


# ── 统计工具 ──────────────────────────────────────────────────────────────────

def _stats(win_series: pd.Series, pnl_series: pd.Series) -> str:
    valid_win = win_series.dropna()
    valid_pnl = pnl_series.dropna()
    if valid_win.empty:
        return "N/A (n=0)"
    wr  = valid_win.mean()
    exp = valid_pnl.mean()
    return f"胜率={wr:.2%}  期望={exp:+.2f}pt  n={len(valid_win)}"


def _print_detail_stats(label: str, win_series: pd.Series, pnl_series: pd.Series) -> None:
    """以 simple_uturn_signal_test 相同格式输出同步集合的完整统计。"""
    win_s = win_series.dropna()
    pnl_s = pnl_series.dropna()
    total = len(win_s)
    if total == 0:
        print(f"  [{label}] N/A (n=0)")
        return
    wins     = int(win_s.sum())
    losses   = total - wins
    win_rate = wins / total
    avg_pnl  = pnl_s.mean()
    avg_win  = pnl_s[win_s.values.astype(bool)].mean() if wins   > 0 else 0.0
    avg_loss = pnl_s[~win_s.values.astype(bool)].mean() if losses > 0 else 0.0
    z_score  = (wins - total * 0.5) / (math.sqrt(total) * 0.5)
    expectancy = win_rate * avg_win + (1 - win_rate) * avg_loss
    W = 60
    print(f"{'─'*W}")
    print(f"  ── 完全同步集合详细统计：{label} ──")
    print(f"  {'signal_type':<20}: {label.split()[0].lower()}")
    print(f"  {'total_signals':<20}: {total}")
    print(f"  {'win_rate':<20}: {win_rate:.2%}")
    print(f"  {'avg_pnl_points':<20}: {avg_pnl:.2f}")
    print(f"  {'avg_win_points':<20}: {avg_win:.2f}")
    print(f"  {'avg_loss_points':<20}: {avg_loss:.2f}")
    print(f"  {'expectancy':<20}: {expectancy:.2f}")
    print(f"  {'z_score':<20}: {z_score:.2f}")


# ── 核心分析 ──────────────────────────────────────────────────────────────────

@dataclass
class LeadLagAnalyzer:
    ids:         list   # 2~3 个品种 ID；ids[0] 为基准（reference）
    interval:    str
    signal_type: str
    window_mins: int    # 同步判定窗口（分钟），建议 ≥ bar 周期
    min_rvold:   float = 0.0  # 仅保留 rvold >= 此值的信号（0=不过滤）

    @property
    def lead_id(self) -> str:
        return self.ids[0]

    @property
    def lag_ids(self) -> list:
        return self.ids[1:]

    def run(self) -> pd.DataFrame:
        lead_df = _load(self.lead_id, self.interval, self.signal_type)
        lag_dfs = {lag: _load(lag, self.interval, self.signal_type) for lag in self.lag_ids}
        if self.min_rvold > 0:
            lead_df = _filter_rvold(lead_df, self.min_rvold)
            lag_dfs = {lag: _filter_rvold(df, self.min_rvold) for lag, df in lag_dfs.items()}
        rows     = [self._match_row(row, lag_dfs) for _, row in lead_df.iterrows()]
        result_df = pd.DataFrame(rows)
        self._print_report(result_df)
        return result_df

    def _match_one(self, t: pd.Timestamp, lag_df: pd.DataFrame) -> dict | None:
        """在窗口内找时间差绝对值最小的 lag 信号，无匹配返回 None。"""
        window     = pd.Timedelta(minutes=self.window_mins)
        candidates = lag_df[(lag_df["datetime"] >= t - window) &
                            (lag_df["datetime"] <= t + window)].copy()
        if candidates.empty:
            return None
        candidates["_abs_diff"] = (candidates["datetime"] - t).abs()
        best      = candidates.loc[candidates["_abs_diff"].idxmin()]
        diff_mins = round((best["datetime"] - t).total_seconds() / 60, 1)
        return {
            "time":        best["datetime"],
            "entry_price": float(best["entry_price"]),
            "diff_mins":   diff_mins,
            "win":         bool(best["is_win"]),
            "pnl":         float(best["pnl_points"]),
            "exit":        best.get("exit_type", ""),
            "rvold":       float(best["rvold"]) if "rvold" in best.index else 0.0,
        }

    def _match_row(self, lead_row: pd.Series, lag_dfs: dict) -> dict:
        t   = lead_row["datetime"]
        row = {
            "lead_time":        t,
            "lead_entry_price": float(lead_row["entry_price"]),
            "lead_win":         bool(lead_row["is_win"]),
            "lead_pnl":         float(lead_row["pnl_points"]),
            "lead_exit":        lead_row.get("exit_type", ""),
            "lead_rvold":       float(lead_row["rvold"]) if "rvold" in lead_row.index else 0.0,
            "is_synced":        False,
        }
        matches: dict[str, dict | None] = {}
        for lag_id, lag_df in lag_dfs.items():
            m = self._match_one(t, lag_df)
            matches[lag_id] = m
            row[f"{lag_id}_time"]        = m["time"]         if m else None
            row[f"{lag_id}_entry_price"] = m["entry_price"]  if m else None
            row[f"{lag_id}_diff_mins"]   = m["diff_mins"]    if m else None
            row[f"{lag_id}_win"]         = m["win"]          if m else None
            row[f"{lag_id}_pnl"]         = m["pnl"]          if m else None
            row[f"{lag_id}_exit"]        = m["exit"]         if m else None
            row[f"{lag_id}_rvold"]       = m["rvold"]        if m else None
        row["is_synced"] = all(m is not None for m in matches.values())
        return row

    def _is_exact_sync(self, df: pd.DataFrame) -> pd.Series:
        """所有 lag 的 diff_mins 均为 0 → 完全同步（同一根 bar）。"""
        mask = pd.Series(True, index=df.index)
        for lag_id in self.lag_ids:
            mask &= df[f"{lag_id}_diff_mins"] == 0
        return mask

    def _print_report(self, df: pd.DataFrame) -> None:
        total      = len(df)
        synced     = df[df["is_synced"]]
        not_synced = df[~df["is_synced"]]
        exact_mask = self._is_exact_sync(synced)
        exact_sync = synced[exact_mask]

        W   = 60
        tag = " ↔ ".join(self.ids) + f"  [{self.signal_type.upper()}  {self.interval}]"
        print(f"\n{'═'*W}")
        print(f"  Lead-Lag Analysis: {tag}")
        print(f"{'═'*W}")
        print(f"  {self.lead_id} 总信号数                    : {total}")
        print(f"{'─'*W}")

        lag_label = " & ".join(self.lag_ids)
        print(f"  窗口内 {lag_label} 全部同步（±{self.window_mins}min）: "
              f"{len(synced)}  ({len(synced)/total:.1%})")

        if len(self.lag_ids) == 1:
            # 两品种：细化先行/跟随/完全同步
            lag_id     = self.lag_ids[0]
            lead_first = synced[synced[f"{lag_id}_diff_mins"] > 0]
            lag_first  = synced[synced[f"{lag_id}_diff_mins"] < 0]
            print(f"    ├ {self.lead_id} 先行（lead < lag）         : "
                  f"{len(lead_first)}  ({len(lead_first)/total:.1%})")
            print(f"    ├ {lag_id}  先行（lag < lead）         : "
                  f"{len(lag_first)}  ({len(lag_first)/total:.1%})")
            print(f"    └ 完全同步（同一根 bar）             : "
                  f"{len(exact_sync)}  ({len(exact_sync)/total:.1%})")
        else:
            # 三品种：只报完全同步数量
            print(f"    └ 完全同步（所有品种同一根 bar）    : "
                  f"{len(exact_sync)}  ({len(exact_sync)/total:.1%})")

        print(f"  窗口内未全部同步                    : "
              f"{len(not_synced)}  ({len(not_synced)/total:.1%})")
        print(f"{'─'*W}")

        # ── 基准品种（lead）胜率 ──
        print(f"  ── {self.lead_id} 胜率/期望 ──")
        print(f"  全部信号      : {_stats(df['lead_win'],            df['lead_pnl'])}")
        print(f"  同步时        : {_stats(synced['lead_win'],        synced['lead_pnl'])}")
        print(f"  完全同步时    : {_stats(exact_sync['lead_win'],    exact_sync['lead_pnl'])}")
        print(f"  未同步时      : {_stats(not_synced['lead_win'],    not_synced['lead_pnl'])}")

        # ── 各 lag 品种同步时胜率 ──
        for lag_id in self.lag_ids:
            print(f"  ── {lag_id} 同步时胜率/期望 ──")
            print(f"  {lag_id} 同步整体  : {_stats(synced[f'{lag_id}_win'],       synced[f'{lag_id}_pnl'])}")
            print(f"  {lag_id} 完全同步  : {_stats(exact_sync[f'{lag_id}_win'],   exact_sync[f'{lag_id}_pnl'])}")

        # 全部品种同时赢（同步集合中）
        if len(synced) > 0:
            all_win = synced["lead_win"].copy()
            for lag_id in self.lag_ids:
                all_win = all_win & (synced[f"{lag_id}_win"] == True)
            bwr = all_win.sum() / len(synced)
            print(f"  全品种同时赢  : {bwr:.2%}  (n={int(all_win.sum())})")

        print(f"{'═'*W}\n")

        # ── 完全同步集合详细统计 ──
        _print_detail_stats(f"{self.lead_id} exact_sync", exact_sync["lead_win"], exact_sync["lead_pnl"])
        for lag_id in self.lag_ids:
            _print_detail_stats(f"{lag_id}  exact_sync",
                                exact_sync[f"{lag_id}_win"], exact_sync[f"{lag_id}_pnl"])
        print(f"{'═'*W}\n")


# ── 主入口 ────────────────────────────────────────────────────────────────────

def main() -> None:
    analyzer = LeadLagAnalyzer(
        ids=["RKLB", "NQ"],          # 2 个品种；改为 ["NQ", "ES", "QQQ"] 即可扩展到 3 个
        interval="M5",
        signal_type="bottom",
        window_mins=1,
        min_rvold = 0,             # 仅保留成交量异常度 >= 1.5 的信号（0=不过滤）
    )
    result_df = analyzer.run()

    label = "_".join(analyzer.ids)
    out   = _OUTPUT_ROOT / f"lead_lag_{label}_{analyzer.signal_type}_{analyzer.interval}.csv"
    result_df.to_csv(out, index=False, float_format="%.4f")
    print(f"明细已导出: {out}")


if __name__ == "__main__":
    main()
