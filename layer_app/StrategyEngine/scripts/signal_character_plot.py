"""
signal_character_plot.py — 信号特征分析图

支持两种数据源：
  source: "signal_test"  从 simple_uturn_signal_test.py 输出的 CSV 读取（全量）
  source: "lead_lag"     从 lead_lag_analysis.py 输出的 CSV 读取（可过滤同步子集）

lead_lag 模式需额外指定：
  instrument:  要分析的品种（"lead" 或 lag 品种名，如 "ES"）
  filter:      "all"（同步+未同步）| "synced"（窗口内同步）| "exact_sync"（同一根 bar）
  lag_ids:     lag 品种列表（用于判断 exact_sync，仅 filter != "all" 时需要）

运行方式：
    cd layer_app/StrategyEngine
    PYTHONPATH=ice:src python3 scripts/signal_character_plot.py
"""
import pickle
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
import yaml

_CONFIG_PATH = Path(__file__).resolve().parents[1] / "config" / "simple_uturn_signal_test.yaml"
_BASE        = Path(__file__).resolve().parents[1]

# ── 硬编码运行配置（按需修改）────────────────────────────────────────────────

SOURCE      = "closed_positions"  # "signal_test" | "lead_lag" | "closed_positions"
SIGNAL_TYPE = "bottom"            # "bottom" | "top"（signal_test 模式使用）
WINDOW_HOURS = 120

# signal_test 模式：从 simple_uturn_signal_test.yaml 读取 code_id / interval
# lead_lag 模式：额外指定以下三项
LEAD_LAG_FILE   = _BASE / "output" / "lead_lag_NQ_ES_bottom_M1.csv"
INSTRUMENT      = "NQ"            # "lead" / "NQ" 或 lag 品种名（如 "ES"）
FILTER_MODE     = "exact_sync"    # "all" | "synced" | "exact_sync"
LAG_IDS         = ["ES"]          # lag 品种列表（exact_sync 判断用）

# closed_positions 模式：指定 auto_test.py / main_backtest.py 输出的成交明细 CSV
CLOSED_POSITIONS_FILE = _BASE / "output" / "NQ" / "deallist" / "UTurnBSync_DebitCallSpread_NQ_M5_LONG.csv"
CLOSED_POSITIONS_CODE_ID = "NQ"   # 用于加载价格缓存


# ── 通用工具 ──────────────────────────────────────────────────────────────────

def _load_yaml_config() -> dict:
    with open(_CONFIG_PATH, encoding="utf-8") as f:
        return yaml.safe_load(f)


def _load_price_df(code_id: str, interval: str, cfg: dict) -> pd.DataFrame:
    start = cfg["start_time"][:10].replace("-", "")
    end   = cfg["end_time"][:10].replace("-", "")
    pkl   = _BASE / "datadb" / f"{code_id}_{interval}_{start}_{end}.pkl"
    if not pkl.exists():
        raise FileNotFoundError(f"价格缓存不存在: {pkl}")
    with open(pkl, "rb") as f:
        bars = pickle.load(f)
    df = pd.DataFrame({"datetime": [b.datetime for b in bars],
                       "close":    [b.close_price for b in bars]})
    df["datetime"] = pd.to_datetime(df["datetime"])
    return df


# ── 数据源加载 ────────────────────────────────────────────────────────────────

def _load_from_signal_test(cfg: dict, signal_type: str) -> tuple[pd.DataFrame, str, int]:
    """返回 (signals_df, code_id, signal_direction)。signals_df 含 entry_time / entry_price / pnl_points。"""
    code_id = cfg["code_id"]
    interval = cfg["interval"]
    path = (_BASE / "output" / code_id / "signal_test"
            / f"simple_uturn_signal_test_{code_id}_{interval}_{signal_type}.csv")
    if not path.exists():
        raise FileNotFoundError(f"信号文件不存在: {path}")
    df = pd.read_csv(path, parse_dates=["datetime"])
    df = df.rename(columns={"datetime": "entry_time"})
    direction = int(df["signal_direction"].iloc[0]) if "signal_direction" in df.columns else 1
    return df, code_id, direction


def _load_from_lead_lag(path: Path, instrument: str, filter_mode: str,
                        lag_ids: list[str]) -> tuple[pd.DataFrame, str, int]:
    """返回 (signals_df, code_id, signal_direction)。signals_df 含 entry_time / entry_price / pnl_points。"""
    raw = pd.read_csv(path, parse_dates=["lead_time"])

    # 过滤
    if filter_mode == "synced":
        raw = raw[raw["is_synced"]].copy()
    elif filter_mode == "exact_sync":
        mask = raw["is_synced"].copy()
        for lag_id in lag_ids:
            mask = mask & (raw[f"{lag_id}_diff_mins"] == 0)
        raw = raw[mask].copy()

    if raw.empty:
        raise ValueError(f"过滤后无记录（filter={filter_mode}）")

    # 提取目标品种列
    lead_id = path.stem.split("_")[2]   # lead_lag_NQ_ES_bottom_M1 → NQ
    if instrument in ("lead", lead_id):
        df = pd.DataFrame({
            "entry_time":  raw["lead_time"],
            "entry_price": raw["lead_entry_price"],
            "pnl_points":  raw["lead_pnl"],
        })
        code_id = lead_id
        direction = 1
    else:
        df = pd.DataFrame({
            "entry_time":  pd.to_datetime(raw[f"{instrument}_time"]),
            "entry_price": raw[f"{instrument}_entry_price"],
            "pnl_points":  raw[f"{instrument}_pnl"],
        })
        code_id = instrument
        direction = 1

    return df.dropna(subset=["entry_time", "entry_price"]).reset_index(drop=True), code_id, direction


def _load_from_closed_positions(path: Path, code_id: str) -> tuple[pd.DataFrame, str, int]:
    """从 auto_test / main_backtest 输出的成交明细 CSV 读取。
    需要列：entry_dt、entry_underlying、final_pnl。
    """
    if not path.exists():
        raise FileNotFoundError(f"成交明细文件不存在: {path}")
    raw = pd.read_csv(path, parse_dates=["entry_dt"])
    df = pd.DataFrame({
        "entry_time":  raw["entry_dt"],
        "entry_price": raw["entry_underlying"],
        "pnl_points":  raw["final_pnl"],
    })
    return df.dropna(subset=["entry_time", "entry_price"]).reset_index(drop=True), code_id, 1


# ── 绘图 ──────────────────────────────────────────────────────────────────────

def _generate_plot(signals_df: pd.DataFrame, price_df: pd.DataFrame,
                   code_id: str, signal_type: str, direction: int,
                   source_label: str, window_hours: int = 120) -> pd.DataFrame:
    mfe_results = []
    mae_results = []
    for _, signal in signals_df.iterrows():
        end_time = signal["entry_time"] + pd.Timedelta(hours=window_hours)
        mask = ((price_df["datetime"] >= signal["entry_time"]) &
                (price_df["datetime"] <= end_time))
        future = price_df[mask].copy()
        if future.empty:
            continue
        future["diff"] = (future["close"] - signal["entry_price"]) * direction
        idx_max = future["diff"].idxmax()
        idx_min = future["diff"].idxmin()
        mfe_results.append({
            "time_to_peak": (future.loc[idx_max, "datetime"] - signal["entry_time"]).total_seconds() / 3600,
            "mfe":          future.loc[idx_max, "diff"],
        })
        mae_results.append({
            "time_to_trough": (future.loc[idx_min, "datetime"] - signal["entry_time"]).total_seconds() / 3600,
            "mae":            future.loc[idx_min, "diff"],
        })

    if not mfe_results:
        print(f"[{code_id}/{signal_type}] 无可分析记录")
        return pd.DataFrame()

    safe_label = source_label.replace(" ", "_").replace("/", "_")
    out_dir = _BASE / "output" / code_id / "signal_test"
    out_dir.mkdir(parents=True, exist_ok=True)
    n = len(mfe_results)

    mfe_df = pd.DataFrame(mfe_results)
    mae_df = pd.DataFrame(mae_results)

    fig, (ax_mfe, ax_mae) = plt.subplots(2, 1, figsize=(12, 12))
    fig.suptitle(f"{code_id} [{signal_type}]  {source_label}  (n={n})", fontsize=13)

    # MFE
    sc1 = ax_mfe.scatter(mfe_df["time_to_peak"], mfe_df["mfe"],
                         alpha=0.6, c=mfe_df["mfe"], cmap="RdYlGn")
    ax_mfe.axvline(x=24, color="red",    linestyle="--", label="T+24h")
    ax_mfe.axvline(x=4,  color="orange", linestyle=":",  label="T+4h")
    fig.colorbar(sc1, ax=ax_mfe, label="MFE (points)")
    ax_mfe.set_title("MFE — Maximum Favorable Excursion")
    ax_mfe.set_xlabel("Hours to reach Peak")
    ax_mfe.set_ylabel("Points")
    ax_mfe.legend()
    ax_mfe.grid(True, alpha=0.3)

    # MAE
    sc2 = ax_mae.scatter(mae_df["time_to_trough"], mae_df["mae"],
                         alpha=0.6, c=mae_df["mae"], cmap="RdYlGn_r")
    ax_mae.axvline(x=24, color="red",    linestyle="--", label="T+24h")
    ax_mae.axvline(x=4,  color="orange", linestyle=":",  label="T+4h")
    ax_mae.axhline(y=0,  color="black",  linestyle="-",  linewidth=0.8)
    fig.colorbar(sc2, ax=ax_mae, label="MAE (points)")
    ax_mae.set_title("MAE — Maximum Adverse Excursion")
    ax_mae.set_xlabel("Hours to reach Trough")
    ax_mae.set_ylabel("Points")
    ax_mae.legend()
    ax_mae.grid(True, alpha=0.3)

    plt.tight_layout()
    save_path = out_dir / f"mfe_mae_{code_id}_{signal_type}_{safe_label}.png"
    plt.savefig(save_path, dpi=150, bbox_inches="tight")
    plt.show()
    print(f"图表已保存: {save_path}")

    return mfe_df


# ── 外部调用接口 ──────────────────────────────────────────────────────────────

def plot_closed_positions(csv_path, bars, code_id: str, window_hours: int = 120) -> None:
    """供 auto_test / main_backtest 调用：从成交明细 CSV + 已加载的 bars 生成信号特征图。"""
    import logging
    _log = logging.getLogger("SignalCharacterPlot")
    csv_path = Path(csv_path)
    _log.info(f"[plot_closed_positions] csv={csv_path}  exists={csv_path.exists()}  window_hours={window_hours}")
    if not csv_path.exists():
        _log.warning(f"CSV 不存在，跳过绘图: {csv_path}")
        return
    raw = pd.read_csv(csv_path, parse_dates=["entry_dt"])
    _log.info(f"CSV 读取完毕，行数={len(raw)}，列={list(raw.columns)}")
    if "entry_underlying" not in raw.columns or raw.empty:
        _log.warning("CSV 缺少 entry_underlying 列或为空，跳过绘图")
        return
    signals_df = pd.DataFrame({
        "entry_time":  raw["entry_dt"],
        "entry_price": raw["entry_underlying"],
        "pnl_points":  raw["final_pnl"],
    }).dropna(subset=["entry_time", "entry_price"]).reset_index(drop=True)
    _log.info(f"signals_df 行数={len(signals_df)}")
    if signals_df.empty:
        _log.warning("signals_df 为空，跳过绘图")
        return
    price_df = pd.DataFrame({"datetime": [b.datetime for b in bars],
                             "close":    [b.close_price for b in bars]})
    price_df["datetime"] = pd.to_datetime(price_df["datetime"])
    try:
        _generate_plot(signals_df, price_df, code_id, "backtest", 1, "backtest", window_hours)
    except Exception as e:
        _log.error(f"_generate_plot 异常: {e}", exc_info=True)


# ── 主入口 ────────────────────────────────────────────────────────────────────

def main() -> None:
    cfg = _load_yaml_config()

    if SOURCE == "signal_test":
        signals_df, code_id, direction = _load_from_signal_test(cfg, SIGNAL_TYPE)
        source_label = "full"
    elif SOURCE == "lead_lag":
        signals_df, code_id, direction = _load_from_lead_lag(
            LEAD_LAG_FILE, INSTRUMENT, FILTER_MODE, LAG_IDS)
        instr_label = "NQ" if INSTRUMENT in ("lead", "NQ") else INSTRUMENT
        source_label = f"{instr_label}_{FILTER_MODE}"
    else:  # closed_positions
        signals_df, code_id, direction = _load_from_closed_positions(
            CLOSED_POSITIONS_FILE, CLOSED_POSITIONS_CODE_ID)
        source_label = "backtest"

    print(f"[{code_id}] 加载 {len(signals_df)} 条记录（{source_label}）")
    price_df = _load_price_df(code_id, cfg["interval"], cfg)
    _generate_plot(signals_df, price_df, code_id, SIGNAL_TYPE, direction,
                   source_label, WINDOW_HOURS)


if __name__ == "__main__":
    main()
