"""
统计开仓机会频率。
读取 strategies.yaml，对每个 enable=true 的策略扫描全部 bar，
记录触发信号的时刻（不实际开仓、不受 max_positions 限制），
输出 CSV 并绘制每周 / 每月信号次数直方图。
"""
import logging
import pickle
import sys
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from qib_trader.core.factory_strategy import FactoryStrategy
from qib_trader.utils.config_loader import get_strategies_config
from qib_trader.utils.log_setup import setup_logging

logger = logging.getLogger("SignalStats")

_DATADB = Path(__file__).resolve().parents[1] / "datadb"
_OUTPUT = Path(__file__).resolve().parents[1] / "output1"


# ── 数据 ──────────────────────────────────────────────────────────────────────

def _cache_path(code_id: str, interval_str: str, start_time: str, end_time: str) -> Path:
    s = str(start_time)[:10].replace("-", "")
    e = str(end_time)[:10].replace("-", "")
    return _DATADB / f"{code_id}_{interval_str}_{s}_{e}.pkl"


def _load_bars(code_id: str, interval_str: str, start_time: str, end_time: str) -> list:
    cache = _cache_path(code_id, interval_str, start_time, end_time)
    if not cache.exists():
        raise FileNotFoundError(
            f"缓存不存在: {cache}，请先运行 main_backtest.py 以生成数据缓存"
        )
    with open(cache, "rb") as f:
        return pickle.load(f)


# ── 信号扫描 ──────────────────────────────────────────────────────────────────

def _patch_strategy(strategy, signals: list) -> None:
    """将 _check_entry 替换为只记录信号时刻，不开仓、不写 _spreads。"""

    def _signal_only_check_entry(bar):
        if strategy._entry_signal(bar):
            signals.append(bar.datetime)
            strategy._last_entry_dt = bar.datetime  # 保留冷却逻辑

    strategy._check_entry = _signal_only_check_entry
    strategy._send_order  = lambda *a, **kw: None   # 防止 on_stop 产生干扰


def _scan_signals(s_config: dict) -> tuple[str, list]:
    params = dict(s_config.get("params", {}))
    params["max_positions"]  = 0    # 不限并发持仓
    params["cooldown_hours"] = 0    # 不限冷却时间

    strategy = FactoryStrategy.create_strategy({**s_config, "params": params})

    signals: list = []
    _patch_strategy(strategy, signals)

    bars = _load_bars(
        s_config["code_id"], s_config["interval"],
        s_config["start_time"], s_config["end_time"],
    )

    strategy.on_init()
    strategy.on_start()
    for bar in bars:
        strategy.on_bar(bar)
    strategy.on_stop()

    logger.info(f"{strategy.strategy_id}: 共 {len(signals)} 个信号")
    return strategy.strategy_id, signals


# ── 保存 CSV ──────────────────────────────────────────────────────────────────

def _save_csv(strategy_id: str, code_id: str, signals: list) -> Path:
    d = _OUTPUT / code_id / "signals"
    d.mkdir(parents=True, exist_ok=True)
    path = d / f"{strategy_id}_signals.csv"
    pd.DataFrame({"signal_dt": signals}).to_csv(path, index=False)
    logger.info(f"信号列表已保存: {path}")
    return path


# ── 绘图 ──────────────────────────────────────────────────────────────────────

def _bar_chart(ax, index, values, color, title, ylabel):
    x = range(len(values))
    ax.bar(x, values, color=color, width=0.8)
    step = max(1, len(index) // 12)
    ax.set_xticks(list(x)[::step])
    ax.set_xticklabels([str(p) for p in index[::step]], rotation=45, ha="right", fontsize=7)
    ax.set_ylabel(ylabel)
    mean = values.mean()
    ax.axhline(mean, color="red", linestyle="--", linewidth=0.8, label=f"均值 {mean:.1f}")
    ax.set_title(title)
    ax.legend(fontsize=8)


def _plot(strategy_id: str, code_id: str, signals: list,
          start_time: str, end_time: str) -> None:
    df = pd.DataFrame({"dt": pd.to_datetime(signals)})

    full_weeks  = pd.period_range(start=start_time, end=end_time, freq="W")
    full_months = pd.period_range(start=start_time, end=end_time, freq="M")

    raw_weekly  = df.groupby(df["dt"].dt.to_period("W")).size() if not df.empty else pd.Series(dtype=int)
    raw_monthly = df.groupby(df["dt"].dt.to_period("M")).size() if not df.empty else pd.Series(dtype=int)

    weekly  = raw_weekly.reindex(full_weeks,  fill_value=0)
    monthly = raw_monthly.reindex(full_months, fill_value=0)

    fig, axes = plt.subplots(2, 1, figsize=(14, 8))
    fig.suptitle(f"开仓机会频率 — {strategy_id}  (总计 {len(signals)} 次)", fontsize=13)

    _bar_chart(axes[0], weekly.index,  weekly,
               "steelblue",
               f"每周信号次数（{len(weekly)} 周，avg={weekly.mean():.1f}次/周）",
               "次数")
    _bar_chart(axes[1], monthly.index, monthly,
               "darkorange",
               f"每月信号次数（{len(monthly)} 月，avg={monthly.mean():.1f}次/月）",
               "次数")

    plt.tight_layout()

    d = _OUTPUT / code_id / "signals"
    d.mkdir(parents=True, exist_ok=True)
    path = d / f"{strategy_id}_signal_freq.png"
    plt.savefig(path, dpi=150, bbox_inches="tight")
    plt.show()
    logger.info(f"图表已保存: {path}")


# ── 主程序 ────────────────────────────────────────────────────────────────────

def main() -> None:
    setup_logging()
    logging.getLogger("Strategy").setLevel(logging.WARNING)

    active = [c for c in get_strategies_config() if c.get("enable", False)]
    if not active:
        logger.warning("strategies.yaml 中无 enable=true 的策略")
        return

    for s_config in active:
        try:
            strategy_id, signals = _scan_signals(s_config)
            _save_csv(strategy_id, s_config["code_id"], signals)
            _plot(strategy_id, s_config["code_id"], signals,
                  s_config["start_time"], s_config["end_time"])
        except Exception as exc:
            logger.error(f"策略 {s_config['class']} 处理失败: {exc}", exc_info=True)


if __name__ == "__main__":
    main()
