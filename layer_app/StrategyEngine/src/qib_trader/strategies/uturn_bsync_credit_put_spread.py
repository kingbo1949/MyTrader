"""
UTurnBSync_CreditPutSpread 策略：
多品种 UTurnBottom 信号同步 → 开仓 Bull Put Spread（Credit Put Spread）→ 持有至到期结算。

uturn_btm_level 控制同步信号等级：
  1 = SimpleUBtm      : is_uturn=True + Bottom/BottomSub
  2 = DbSimpleBtm     : 窗口内 ≥2 个底部 UTurn 点（不要求 MACD>0）
  3 = MixDbSimpleBtm  : DbSimpleBtm + 至少一根 bar 的 MACD>0

同步时间戳集合在 on_start 阶段预构建（取各品种信号时间戳的交集），
logic() 只做 O(1) 查找，不影响回测性能。
"""
import os
import pickle
from collections import deque
from datetime import timedelta, datetime, time as dtime
from pathlib import Path


from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing

_DATADB = Path(__file__).parents[3] / "datadb"


# ── BS 定价 ───────────────────────────────────────────────────────────────────


# ── 同步时间戳构建 ────────────────────────────────────────────────────────────

def _load_bars(code_id: str, interval_str: str, start: str, end: str) -> list:
    s = start[:10].replace("-", "")
    e = end[:10].replace("-", "")
    cache = _DATADB / f"{code_id}_{interval_str}_{s}_{e}.pkl"
    if cache.exists():
        try:
            with open(cache, "rb") as f:
                return pickle.load(f)
        except Exception:
            cache.unlink(missing_ok=True)   # 文件损坏/截断，删除后重新拉取
    try:
        import IBTrader
        from qib_trader.api.converters import IceConverter
        from qib_trader.utils.factory_ice import FactoryIce
        time_type = getattr(IBTrader.ITimeType, interval_str)
        time_pair = FactoryIce.make_timepair(start, end)
        df = FactoryIce.get_enriched_klines_loop(code_id, time_type, time_pair)
        if df.empty:
            return []
        bars = IceConverter.enriched_df_to_bars(df, code_id, interval_str)
        _DATADB.mkdir(exist_ok=True)
        # 原子写入：每个进程用独立 tmp 文件（含 PID），rename 到最终路径
        # 多进程并发时各自写各自的 tmp，最后一个 rename 成功即可，均为完整数据
        tmp = cache.with_name(f"{cache.stem}_{os.getpid()}.tmp")
        with open(tmp, "wb") as f:
            pickle.dump(bars, f)
        os.replace(tmp, cache)
        return bars
    except Exception as exc:
        raise RuntimeError(f"无法加载 {code_id} {interval_str} 数据: {exc}")


def _uturn_bottom_ts_level1(bars: list, min_dif_abs: float) -> set:
    return {
        bar.datetime for bar in bars
        if bar.is_uturn
        and bar.div_type in ('Bottom', 'BottomSub')
        and abs(bar.dif) >= min_dif_abs
    }


def _uturn_bottom_ts_multi(bars: list, min_dif_abs: float, level: int, window: int) -> set:
    """Level 2/3：顺序扫描，每 bar 对历史窗口调用 detect_multi_uturn。"""
    history, result = deque(maxlen=window), set()
    for bar in bars:
        history.append(bar)
        if min_dif_abs > 0 and abs(bar.dif) < min_dif_abs:
            continue
        zone = detect_multi_uturn(list(history))
        if zone is None or zone.zone_type != 'bottom':
            continue
        if level == 3 and not zone.is_mix:
            continue
        result.add(bar.datetime)
    return result


def _build_sync_timestamps(sync_signals: list, interval_str: str,
                            start: str, end: str,
                            level: int = 1, window: int = 50) -> set:
    """对 sync_signals 中每个品种按指定等级计算时间戳集合，取交集。"""
    result: set | None = None
    for sig in sync_signals:
        bars      = _load_bars(sig['code_id'], interval_str, start, end)
        dif_floor = float(sig.get('min_dif_abs', 0.0))
        if level == 1:
            ts = _uturn_bottom_ts_level1(bars, dif_floor)
        else:
            ts = _uturn_bottom_ts_multi(bars, dif_floor, level, window)
        result = ts if result is None else result & ts
    return result if result is not None else set()


# ── 策略主体 ──────────────────────────────────────────────────────────────────

class UTurnBSync_CreditPutSpread(Strategy):
    """
    多品种简单UTurnBottom同步信号 → Bull Put Spread。
    sync_signals 中所有品种在同一根 bar 同时触发简单UTurnBottom时开仓。
    """

    options_mode: bool  = True
    initial_capital: float = 100_000.0

    sync_signals:  list  = []                 # [{code_id, min_dif_abs}, ...]
    start_time:    str   = "2024-01-01 00:00:00"
    end_time:      str   = "2026-06-01 00:00:00"

    uturn_btm_level:  int   = 1    # 信号等级：1=SimpleUBtm 2=DbSimpleBtm 3=MixDbSimpleBtm
    multi_uturn_window: int = 50   # Level 2/3 使用的 bar 历史窗口大小
    min_dif_abs:   float = 0.0    # 交易标的开仓 bar 的 |dif| 最小值，低于此值忽略（0=不限制）
    entry_short:   float = 0.4
    entry_long:    float = 0.1
    option_days:   int   = 5
    expiry_weekday: int  = -1
    risk_free_rate: float = 0.04
    strike_round:  float = 100.0
    cooldown_hours: float = 1.0
    max_positions: int   = 3
    hard_stop_points:     float = 0.0
    price_check_hours:    float = 0.0  # 价格检查：开仓后多少小时做一次性检查（0=不启用）
    min_price_gain_ratio: float = 0.0  # 价格检查：标的涨幅低于此比例则平仓（支持负值）
    early_profit_ratio:   float = 0.0
    early_time_ratio:     float = 0.0
    late_close_time_ratio: float = 0.0  # 时间强制平仓：已用时间/总持仓时间超过此比例时无条件平仓，0=不启用
    min_net_credit:       float = 0.0   # 最低净权利金（低于此值不开仓，0=不限制）

    def on_init(self) -> None:
        self._spreads: list[dict] = []
        self._closed_positions: list[dict] = []
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._sync_timestamps: set = set()

    def on_start(self) -> None:
        self.active = True
        if not self.sync_signals:
            self.logger.warning("sync_signals 为空，策略永远不会开仓")
            return
        self.logger.info(f"构建同步时间戳，品种数: {len(self.sync_signals)}")
        self._sync_timestamps = _build_sync_timestamps(
            self.sync_signals, self.interval.value,
            self.start_time, self.end_time,
            level=self.uturn_btm_level, window=self.multi_uturn_window,
        )
        level_names = {1: 'SimpleUBtm', 2: 'DbSimpleBtm', 3: 'MixDbSimpleBtm'}
        self.logger.info(
            f"UTurnBottom同步时间戳共 {len(self._sync_timestamps)} 个"
            f"（等级: {level_names.get(self.uturn_btm_level, self.uturn_btm_level)}）"
        )

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for sp in self._spreads:
            self._settle_spread(sp, self._last_bar, 'force_close')
        self._spreads.clear()
        # 清仓后重置 MTM 为 0，避免 equity 曲线在最后一 bar 出现双重计数
        # （on_stop 在 logic() 之后执行，_mtm_pnl_series[last_bar] 此前已记录浮动盈亏，
        #  而 force_close 又在同一 bar 结算实现盈亏，二者相加会造成重复）
        self._record_mtm_pnl(self._last_bar)

    # ── 工具 ──────────────────────────────────────────────────────────────────

    def _put_strike_by_delta(self, S: float, T: float, put_delta_abs: float, iv: float) -> float:
        return option_pricing.put_strike_by_delta(
            S, T, put_delta_abs, iv, self.risk_free_rate, self.codeId, self.strike_round)

    def _next_expiry(self, bar_dt: datetime, min_days: int) -> datetime:
        if min_days == 1:
            target = bar_dt + timedelta(days=1)
            extra  = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        elif self.expiry_weekday == -1:
            target = bar_dt + timedelta(days=min_days)
            extra  = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        else:
            target = bar_dt + timedelta(days=min_days)
            days_ahead  = (self.expiry_weekday - target.weekday()) % 7
            expiry_date = (target + timedelta(days=days_ahead)).date()
        expiry = datetime.combine(expiry_date, dtime(16, 0))
        return expiry

    def _rem_t(self, bar_dt: datetime, expiry: datetime) -> float:
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    # ── 持仓管理 ──────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        remaining = []
        for sp in self._spreads:
            if bar.datetime >= sp['expiry']:
                self._settle_spread(sp, bar)
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _settle_spread(self, sp: dict, bar: BarData, close_reason: str = 'expiry') -> None:
        S            = bar.close_price
        short_settle = max(sp['k1'] - S, 0.0)
        long_settle  = max(sp['k2'] - S, 0.0)
        payoff       = long_settle - short_settle
        final_pnl    = sp['net_credit'] + payoff
        self._send_order(Direction.LONG, sp['net_credit'] - final_pnl, 1, "MKT")
        self._record_position(sp, bar, close_reason, short_settle, long_settle, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] spread到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} final_pnl={final_pnl:.2f}"
        )

    def _record_position(self, sp: dict, bar: BarData, close_reason: str,
                         short_settle: float, long_settle: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':         sp['entry_dt'],
            'entry_underlying': sp['entry_underlying'],
            'settlement_dt':    bar.datetime,
            'close_reason':     close_reason,
            'k1':               sp['k1'],
            'k2':               sp['k2'],
            'short_prem':       sp['short_prem'],
            'long_prem':        sp['long_prem'],
            'net_credit':       sp['net_credit'],
            'short_settle':     short_settle,
            'long_settle':      long_settle,
            'final_pnl':        final_pnl,
        })

    def _settle_stop_loss(self, sp: dict, bar: BarData,
                          close_reason: str = 'stop_loss',
                          settle_price: float | None = None) -> None:
        T           = self._rem_t(bar.datetime, sp['expiry'])
        S           = settle_price if settle_price is not None else bar.close_price
        curr_p1     = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
        curr_p2     = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
        curr_spread = curr_p1 - curr_p2
        final_pnl   = sp['net_credit'] - curr_spread
        self._send_order(Direction.LONG, curr_spread, 1, "MKT")
        self._record_position(sp, bar, close_reason, 0.0, 0.0, final_pnl)
        self.logger.info(f"[{bar.datetime}] {close_reason} S={S:.2f} "
                         f"K1={sp['k1']} K2={sp['k2']} "
                         f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")

    def _check_hard_stop(self, bar: BarData) -> None:
        if self.hard_stop_points <= 0:
            return
        remaining = []
        for sp in self._spreads:
            stop_price = sp['entry_underlying'] - self.hard_stop_points
            if bar.low_price < stop_price:
                self._settle_stop_loss(sp, bar, close_reason='hard_stop',
                                       settle_price=stop_price)
                self._last_entry_dt = bar.datetime
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _settle_early_profit(self, sp: dict, bar: BarData) -> None:
        T           = self._rem_t(bar.datetime, sp['expiry'])
        S           = bar.close_price
        curr_p1     = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
        curr_p2     = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
        curr_spread = curr_p1 - curr_p2
        final_pnl   = sp['net_credit'] - curr_spread
        self._send_order(Direction.LONG, curr_spread, 1, "MKT")
        self._record_position(sp, bar, 'early_profit', 0.0, 0.0, final_pnl)
        self.logger.info(f"[{bar.datetime}] 提前止盈 K1={sp['k1']} K2={sp['k2']} "
                         f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")

    def _check_price_progress(self, bar: BarData) -> None:
        if self.price_check_hours <= 0:
            return
        remaining = []
        for sp in self._spreads:
            elapsed_h = (bar.datetime - sp['entry_dt']).total_seconds() / 3600
            if elapsed_h < self.price_check_hours or sp['price_checked']:
                remaining.append(sp)
                continue
            sp['price_checked'] = True
            gain_ratio = (bar.close_price - sp['entry_underlying']) / sp['entry_underlying']
            if gain_ratio < self.min_price_gain_ratio:
                self._settle_stop_loss(sp, bar, 'price_progress_stop')
                self.logger.info(
                    f"[{bar.datetime}] 价格检查止损 K1={sp['k1']} K2={sp['k2']} "
                    f"gain_ratio={gain_ratio:.4f} threshold={self.min_price_gain_ratio:.4f}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_early_profit(self, bar: BarData) -> None:
        if self.early_profit_ratio <= 0 or self.early_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs >= self.early_time_ratio:
                remaining.append(sp)
                continue
            T = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
            curr_profit = sp['net_credit'] - (curr_p1 - curr_p2)
            if sp['net_credit'] > 0 and curr_profit >= self.early_profit_ratio * sp['net_credit']:
                self._settle_early_profit(sp, bar)
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_late_close(self, bar: BarData) -> None:
        if self.late_close_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs > 0 and elapsed_secs / total_secs >= self.late_close_time_ratio:
                T = self._rem_t(bar.datetime, sp['expiry'])
                curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
                curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'], self.codeId)
                curr_spread = curr_p1 - curr_p2
                final_pnl = sp['net_credit'] - curr_spread
                self._send_order(Direction.LONG, curr_spread, 1, "MKT")
                self._record_position(sp, bar, 'late_close', 0.0, 0.0, final_pnl)
                self.logger.info(f"[{bar.datetime}] 时间平仓 K1={sp['k1']} K2={sp['k2']} "
                                 f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")
            else:
                remaining.append(sp)
        self._spreads = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        """Credit Put Spread MTM：Σ (收到净权利金 - 当前回购成本)，盈正亏负。"""
        if not self._spreads:
            return 0.0
        S, total = bar.close_price, 0.0
        for sp in self._spreads:
            T       = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            total  += sp['net_credit'] - (curr_p1 - curr_p2)
        return total

    # ── 开仓信号 ──────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if bar.datetime not in self._sync_timestamps:
            return False
        if self.min_dif_abs > 0 and abs(bar.dif) < self.min_dif_abs:
            return False
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.max_positions > 0 and len(self._spreads) >= self.max_positions:
            return False
        return True

    def _execute_spread_entry(self, bar: BarData) -> None:
        expiry     = self._next_expiry(bar.datetime, self.option_days)
        T          = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv_short   = get_iv(self.codeId, 'put', self.entry_short)
        iv_long    = get_iv(self.codeId, 'put', self.entry_long)
        k1         = self._put_strike_by_delta(bar.close_price, T, self.entry_short, iv_short)
        k2         = self._put_strike_by_delta(bar.close_price, T, self.entry_long,  iv_long)
        p1         = option_pricing.put_price(bar.close_price, k1, T, self.risk_free_rate, iv_short, self.codeId)
        p2         = option_pricing.put_price(bar.close_price, k2, T, self.risk_free_rate, iv_long, self.codeId)
        net_credit = p1 - p2
        if self.min_net_credit > 0 and net_credit < self.min_net_credit:
            self.logger.debug(
                f"[{bar.datetime}] net_credit={net_credit:.4f} < "
                f"min_net_credit={self.min_net_credit}，跳过开仓"
            )
            return
        self._last_entry_dt = bar.datetime
        self._spreads.append({
            'entry_dt':         bar.datetime,
            'entry_underlying': bar.close_price,
            'k1': k1, 'k2': k2,
            'short_prem': p1, 'long_prem': p2, 'net_credit': net_credit,
            'expiry':     expiry,
            'iv_short':   iv_short, 'iv_long': iv_long,
            'price_checked': False,
        })
        self._send_order(Direction.SHORT, net_credit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓BullPutSpread K1={k1} K2={k2} "
            f"net_credit={net_credit:.2f} expiry={expiry.date()}"
        )

    def _check_entry(self, bar: BarData) -> None:
        if not self._entry_signal(bar):
            return
        self._execute_spread_entry(bar)

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_expiry(bar)
        self._check_hard_stop(bar)
        self._check_price_progress(bar)
        self._check_early_profit(bar)
        self._check_late_close(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
