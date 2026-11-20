"""
UTurnBSync_DebitPutSpread 策略：
多品种 UTurnTop 信号同步 → 开仓 Bear Put Spread（Debit Put Spread）→ 持有至到期结算。
是 UTurnBSync_DebitCallSpread 的镜像做空版本。

uturn_top_level 控制同步信号等级：
  1 = SimpleUTop      : is_uturn=True + Top/TopSub
  2 = DbSimpleTop     : 窗口内 ≥2 个顶部 UTurn 点（不要求 MACD<0）
  3 = MixDbSimpleTop  : DbSimpleTop + 至少一根 bar 的 MACD<0

开仓结构：
  entry_long  delta 对应的 put 为高行权价 K1（买入腿，高 |delta|，较接近平值）
  entry_short delta 对应的 put 为低行权价 K2（卖出腿，低 |delta|，更虚值）
  K1 > K2
  net_debit  = long_prem - short_prem（开仓净支出）
  max_profit = (K1 - K2) - net_debit（标的跌至 K2 以下时）
  max_loss   = net_debit
"""
from collections import deque
from datetime import timedelta, datetime, time as dtime

from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies.uturn_bsync_credit_put_spread import _load_bars
from qib_trader.utils.bar_patterns import detect_multi_uturn
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing


# ── 顶部同步时间戳构建 ────────────────────────────────────────────────────────

def _uturn_top_ts_level1(bars: list, min_dif_abs: float) -> set:
    """Level 1：bar 级别顶部 UTurn（is_uturn=True + Top/TopSub）。"""
    return {
        bar.datetime for bar in bars
        if bar.is_uturn
        and bar.div_type in ('Top', 'TopSub')
        and abs(bar.dif) >= min_dif_abs
    }


def _uturn_top_ts_multi(bars: list, min_dif_abs: float, level: int, window: int) -> set:
    """Level 2/3：顺序扫描，每 bar 对历史窗口调用 detect_multi_uturn。"""
    history, result = deque(maxlen=window), set()
    for bar in bars:
        history.append(bar)
        if min_dif_abs > 0 and abs(bar.dif) < min_dif_abs:
            continue
        zone = detect_multi_uturn(list(history))
        if zone is None or zone.zone_type != 'top':
            continue
        if level == 3 and not zone.is_mix:
            continue
        result.add(bar.datetime)
    return result


def _build_sync_timestamps_top(sync_signals: list, interval_str: str,
                                start: str, end: str,
                                level: int = 1, window: int = 50) -> set:
    """对 sync_signals 中每个品种按指定等级计算顶部时间戳集合，取交集。"""
    result: set | None = None
    for sig in sync_signals:
        bars      = _load_bars(sig['code_id'], interval_str, start, end)
        dif_floor = float(sig.get('min_dif_abs', 0.0))
        ts = (_uturn_top_ts_level1(bars, dif_floor) if level == 1
              else _uturn_top_ts_multi(bars, dif_floor, level, window))
        result = ts if result is None else result & ts
    return result if result is not None else set()


# ── 策略主体 ──────────────────────────────────────────────────────────────────

class UTurnBSync_DebitPutSpread(Strategy):
    """
    多品种简单 UTurnTop 同步信号 → Bear Put Spread（Debit Put Spread）。
    sync_signals 中所有品种在同一根 bar 同时触发 UTurnTop 时开仓。
    """

    options_mode: bool    = True
    initial_capital: float = 100_000.0

    sync_signals:    list  = []
    start_time:      str   = "2024-01-01 00:00:00"
    end_time:        str   = "2026-06-01 00:00:00"

    uturn_top_level:    int   = 1      # 信号等级：1=SimpleUTop 2=DbSimpleTop 3=MixDbSimpleTop
    multi_uturn_window: int   = 50     # Level 2/3 使用的 bar 历史窗口大小
    min_dif_abs:        float = 0.0    # 交易标的开仓 bar 的 |dif| 最小值（0=不限制）

    entry_long:     float = 0.75      # 买入腿 |put delta|（K1，高行权价，接近平值）
    entry_short:    float = 0.20      # 卖出腿 |put delta|（K2，低行权价，更虚值）
    option_days:    int   = 7
    expiry_weekday: int   = -1        # -1=仅跳过周末；0=周一…4=周五
    risk_free_rate: float = 0.04
    strike_round:   float = 100.0
    cooldown_hours: float = 1.0
    max_positions:  int   = 3

    entry_start_hour:     float = 0.0    # 开仓时间窗口起始（小时，如 6.5 = 06:30；0=不限制）
    entry_end_hour:       float = 24.0   # 开仓时间窗口结束（小时，如 9.5 = 09:30；24=不限制）

    hard_stop_points:     float = 0.0   # 标的上涨超此点数触发硬止损（0=不启用）
    price_check_hours:    float = 0.0   # 价格检查：开仓后多少小时做一次性检查（0=不启用）
    min_price_drop_ratio: float = 0.0   # 价格检查：标的下跌幅度低于此比例则平仓（0=不启用）
    mid_min_profit_ratio: float = 0.0   # 中期检查：利润低于此比例×max_profit则平仓（支持负值）
    mid_time_ratio:       float = 0.0   # 中期检查：单次检查时间点（0=不启用）
    early_profit_ratio:   float = 0.0   # 提前止盈：利润达到 max_profit 此比例时触发（0=不启用）
    early_time_ratio:     float = 0.0   # 提前止盈：已用时间/总持仓时间低于此比例时才触发
    late_profit_ratio:    float = 0.0   # 后期止盈：利润达到 max_profit 此比例时触发（0=不启用）
    late_time_ratio:      float = 0.0   # 后期止盈：已用时间/总持仓时间高于此比例时才触发
    stop_loss_ratio:      float = 0.0   # 权利金止损：价差价值亏损超过 net_debit 此比例时平仓（0=不启用）

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
        self.logger.info(f"构建顶部同步时间戳，品种数: {len(self.sync_signals)}")
        self._sync_timestamps = _build_sync_timestamps_top(
            self.sync_signals, self.interval.value,
            self.start_time, self.end_time,
            level=self.uturn_top_level, window=self.multi_uturn_window,
        )
        level_names = {1: 'SimpleUTop', 2: 'DbSimpleTop', 3: 'MixDbSimpleTop'}
        self.logger.info(
            f"UTurnTop 同步时间戳共 {len(self._sync_timestamps)} 个"
            f"（等级: {level_names.get(self.uturn_top_level, self.uturn_top_level)}）"
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

    def _put_strike_by_delta(self, S: float, T: float, delta_abs: float, iv: float) -> float:
        return option_pricing.put_strike_by_delta(
            S, T, delta_abs, iv, self.risk_free_rate, self.codeId, self.strike_round)

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
        """到期结算：payoff = max(K1-S,0) - max(K2-S,0)；final_pnl = payoff - net_debit。"""
        S            = bar.close_price
        long_settle  = max(sp['k1'] - S, 0.0)   # 买入 put（K1）的内在价值
        short_settle = max(sp['k2'] - S, 0.0)   # 卖出 put（K2）被行权的损失
        payoff       = long_settle - short_settle
        final_pnl    = payoff - sp['net_debit']
        self._send_order(Direction.SHORT, payoff, 1, "MKT")
        self._record_position(sp, bar, close_reason, long_settle, short_settle, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] spread到期 K1={sp['k1']} K2={sp['k2']} "
            f"payoff={payoff:.2f} final_pnl={final_pnl:.2f}"
        )

    def _record_position(self, sp: dict, bar: BarData, close_reason: str,
                         long_settle: float, short_settle: float, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':         sp['entry_dt'],
            'entry_underlying': sp['entry_underlying'],
            'settlement_dt':    bar.datetime,
            'close_reason':     close_reason,
            'k1':               sp['k1'],
            'k2':               sp['k2'],
            'long_prem':        sp['long_prem'],
            'short_prem':       sp['short_prem'],
            'net_debit':        sp['net_debit'],
            'max_profit':       sp['max_profit'],
            'long_settle':      long_settle,
            'short_settle':     short_settle,
            'final_pnl':        final_pnl,
        })

    def _settle_stop_loss(self, sp: dict, bar: BarData,
                          close_reason: str = 'stop_loss',
                          settle_price: float | None = None) -> None:
        """以当前市价平仓整个 spread，记录实际每腿价格。"""
        T           = self._rem_t(bar.datetime, sp['expiry'])
        S           = settle_price if settle_price is not None else bar.close_price
        curr_p1     = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
        curr_p2     = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
        curr_spread = curr_p1 - curr_p2
        final_pnl   = curr_spread - sp['net_debit']
        self._send_order(Direction.SHORT, curr_spread, 1, "MKT")
        self._record_position(sp, bar, close_reason, curr_p1, curr_p2, final_pnl)
        self.logger.info(f"[{bar.datetime}] {close_reason} S={S:.2f} "
                         f"K1={sp['k1']} K2={sp['k2']} "
                         f"curr_spread={curr_spread:.2f} final_pnl={final_pnl:.2f}")

    def _check_stop_loss(self, bar: BarData) -> None:
        """权利金止损：价差市价亏损超过 net_debit × stop_loss_ratio 时平仓。"""
        if self.stop_loss_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            T           = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1     = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            curr_p2     = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_spread = curr_p1 - curr_p2
            loss        = sp['net_debit'] - curr_spread
            if loss >= self.stop_loss_ratio * sp['net_debit']:
                self._settle_stop_loss(sp, bar, 'premium_stop_loss')
                self.logger.info(
                    f"[{bar.datetime}] 权利金止损 K1={sp['k1']} K2={sp['k2']} "
                    f"net_debit={sp['net_debit']:.2f} curr_spread={curr_spread:.2f} "
                    f"loss_ratio={loss/sp['net_debit']:.2%}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_hard_stop(self, bar: BarData) -> None:
        """硬止损：标的上涨超过 hard_stop_points 点触发（做空策略，方向与 call spread 相反）。"""
        if self.hard_stop_points <= 0:
            return
        remaining = []
        for sp in self._spreads:
            stop_price = sp['entry_underlying'] + self.hard_stop_points
            if bar.high_price > stop_price:
                self._settle_stop_loss(sp, bar, close_reason='hard_stop',
                                       settle_price=stop_price)
                self._last_entry_dt = bar.datetime
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_price_progress(self, bar: BarData) -> None:
        """价格进度检查：开仓后 price_check_hours 小时，标的下跌幅度不足则平仓。"""
        if self.price_check_hours <= 0:
            return
        remaining = []
        for sp in self._spreads:
            elapsed_h = (bar.datetime - sp['entry_dt']).total_seconds() / 3600
            if elapsed_h < self.price_check_hours or sp['price_checked']:
                remaining.append(sp)
                continue
            sp['price_checked'] = True
            drop_ratio = (sp['entry_underlying'] - bar.close_price) / sp['entry_underlying']
            if drop_ratio < self.min_price_drop_ratio:
                self._settle_stop_loss(sp, bar, 'price_progress_stop')
                self.logger.info(
                    f"[{bar.datetime}] 价格检查止损 K1={sp['k1']} K2={sp['k2']} "
                    f"drop_ratio={drop_ratio:.4f} threshold={self.min_price_drop_ratio:.4f}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_mid_progress(self, bar: BarData) -> None:
        """中期检查：达到 mid_time_ratio 时间点时，利润不足则平仓。"""
        if self.mid_time_ratio <= 0:
            return
        remaining = []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs < self.mid_time_ratio or sp['mid_checked']:
                remaining.append(sp)
                continue
            sp['mid_checked'] = True
            if sp['max_profit'] <= 0:
                remaining.append(sp)
                continue
            T           = self._rem_t(bar.datetime, sp['expiry'])
            S           = bar.close_price
            curr_p1     = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            curr_p2     = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_profit = (curr_p1 - curr_p2) - sp['net_debit']
            if curr_profit < self.mid_min_profit_ratio * sp['max_profit']:
                self._settle_stop_loss(sp, bar, 'mid_progress_stop')
                self.logger.info(
                    f"[{bar.datetime}] 中期止损 K1={sp['k1']} K2={sp['k2']} "
                    f"curr_profit={curr_profit:.2f} threshold={self.mid_min_profit_ratio * sp['max_profit']:.2f}"
                )
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_early_profit(self, bar: BarData) -> None:
        """提前止盈：早期阶段利润达到 max_profit × early_profit_ratio 时平仓。"""
        if self.early_profit_ratio <= 0 or self.early_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs >= self.early_time_ratio:
                remaining.append(sp)
                continue
            if sp['max_profit'] <= 0:
                remaining.append(sp)
                continue
            T           = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1     = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            curr_p2     = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_profit = (curr_p1 - curr_p2) - sp['net_debit']
            if curr_profit >= self.early_profit_ratio * sp['max_profit']:
                self._send_order(Direction.SHORT, curr_p1 - curr_p2, 1, "MKT")
                self._record_position(sp, bar, 'early_profit', curr_p1, curr_p2, curr_profit)
                self.logger.info(f"[{bar.datetime}] 提前止盈 K1={sp['k1']} K2={sp['k2']} "
                                 f"curr_spread={curr_p1-curr_p2:.2f} final_pnl={curr_profit:.2f}")
            else:
                remaining.append(sp)
        self._spreads = remaining

    def _check_late_profit(self, bar: BarData) -> None:
        """后期止盈：晚期阶段利润达到 max_profit × late_profit_ratio 时平仓。"""
        if self.late_profit_ratio <= 0 or self.late_time_ratio <= 0:
            return
        S, remaining = bar.close_price, []
        for sp in self._spreads:
            total_secs   = (sp['expiry'] - sp['entry_dt']).total_seconds()
            elapsed_secs = (bar.datetime - sp['entry_dt']).total_seconds()
            if total_secs <= 0 or elapsed_secs / total_secs < self.late_time_ratio:
                remaining.append(sp)
                continue
            if sp['max_profit'] <= 0:
                remaining.append(sp)
                continue
            T           = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1     = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            curr_p2     = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            curr_profit = (curr_p1 - curr_p2) - sp['net_debit']
            if curr_profit >= self.late_profit_ratio * sp['max_profit']:
                self._send_order(Direction.SHORT, curr_p1 - curr_p2, 1, "MKT")
                self._record_position(sp, bar, 'late_profit', curr_p1, curr_p2, curr_profit)
                self.logger.info(f"[{bar.datetime}] 后期止盈 K1={sp['k1']} K2={sp['k2']} "
                                 f"curr_spread={curr_p1-curr_p2:.2f} final_pnl={curr_profit:.2f}")
            else:
                remaining.append(sp)
        self._spreads = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        """Debit Put Spread MTM：Σ (当前价差市值 - 支付的净权利金)，盈正亏负。"""
        if not self._spreads:
            return 0.0
        S, total = bar.close_price, 0.0
        for sp in self._spreads:
            T       = self._rem_t(bar.datetime, sp['expiry'])
            curr_p1 = option_pricing.put_price(S, sp['k1'], T, self.risk_free_rate, sp['iv_long'],  self.codeId)
            curr_p2 = option_pricing.put_price(S, sp['k2'], T, self.risk_free_rate, sp['iv_short'], self.codeId)
            total  += (curr_p1 - curr_p2) - sp['net_debit']
        return total

    # ── 开仓信号 ──────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if bar.datetime not in self._sync_timestamps:
            return False
        if self.min_dif_abs > 0 and abs(bar.dif) < self.min_dif_abs:
            return False
        bar_hour = bar.datetime.hour + bar.datetime.minute / 60.0
        if not (self.entry_start_hour <= bar_hour < self.entry_end_hour):
            return False
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.max_positions > 0 and len(self._spreads) >= self.max_positions:
            return False
        return True

    def _check_entry(self, bar: BarData) -> None:
        """开仓 Bear Put Spread：同时买入 K1 put（高行权价）和卖出 K2 put（低行权价）。"""
        if not self._entry_signal(bar):
            return
        expiry    = self._next_expiry(bar.datetime, self.option_days)
        T         = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv_long   = get_iv(self.codeId, 'put', self.entry_long)
        iv_short  = get_iv(self.codeId, 'put', self.entry_short)
        k1        = self._put_strike_by_delta(bar.close_price, T, self.entry_long,  iv_long)
        k2        = self._put_strike_by_delta(bar.close_price, T, self.entry_short, iv_short)
        p1        = option_pricing.put_price(bar.close_price, k1, T, self.risk_free_rate, iv_long,  self.codeId)
        p2        = option_pricing.put_price(bar.close_price, k2, T, self.risk_free_rate, iv_short, self.codeId)
        net_debit  = p1 - p2
        max_profit = (k1 - k2) - net_debit
        self._last_entry_dt = bar.datetime
        self._spreads.append({
            'entry_dt':         bar.datetime,
            'entry_underlying': bar.close_price,
            'k1': k1, 'k2': k2,
            'long_prem':     p1, 'short_prem': p2,
            'net_debit':     net_debit,
            'max_profit':    max_profit,
            'expiry':        expiry,
            'iv_long':       iv_long, 'iv_short': iv_short,
            'mid_checked':   False,
            'price_checked': False,
        })
        self._send_order(Direction.LONG, net_debit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 同步信号开仓BearPutSpread K1={k1} K2={k2} "
            f"net_debit={net_debit:.2f} max_profit={max_profit:.2f} expiry={expiry.date()}"
        )

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_expiry(bar)
        self._check_hard_stop(bar)
        self._check_stop_loss(bar)
        self._check_price_progress(bar)
        self._check_mid_progress(bar)
        self._check_early_profit(bar)
        self._check_late_profit(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
