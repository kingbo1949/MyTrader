"""
UTurnBSync_Ratio 策略：
多品种 UTurnBottom 信号同步 → 买正股（Bottom 开仓）+ 1:2 Ratio Call Spread。
Ratio Call Spread 的开仓时机由参数 ratio_entry_signal 决定：
  "bottom" — 与正股同步在 UTurnBottom 开仓（默认）
  "top"    — 正股在 UTurnBottom 开仓，Ratio 等到下一个 UTurnTop 再开仓

开仓结构：
  buy 1 unit underlying at S0           （UTurnBottom 开仓）
  buy 1 ATM  call at strike K1, prem p1  （由 entry_long  delta 决定，K1 ≈ S）
  sell 2 OTM call at strike K2, prem p2  （由 entry_short delta 决定，K2 > K1）

net_debit = p1 - 2×p2（通常接近0或小额借方，按开仓 Ratio 时的标的价格计算）

────────────────────────────────────────────────────────────────────
到期盈亏 / 合成订单
────────────────────────────────────────────────────────────────────
模式 "bottom"（LONG @ S0+net_debit）：
  S < K1      → pnl = (S−S0)−net_debit          SHORT @ S
  K1≤S<K2     → pnl = (2S−K1−S0)−net_debit      SHORT @ 2S−K1
  S≥K2        → pnl = (2K2−K1−S0)−net_debit     SHORT @ 2K2−K1

模式 "top"（LONG @ S0，Ratio成本内嵌在 EXIT 价格）：
  S < K1      → pnl = (S−S0)−net_debit          SHORT @ S−net_debit
  K1≤S<K2     → pnl = (2S−K1−S0)−net_debit      SHORT @ 2S−K1−net_debit
  S≥K2        → pnl = (2K2−K1−S0)−net_debit     SHORT @ 2K2−K1−net_debit

Ratio 未开仓时提前平仓（top模式）：
  pnl = curr_S − S0                             SHORT @ curr_S

提前平仓（Ratio 已开）MTM：
  pnl = (S−S0)+(c1−p1)−2(c2−p2)
  "bottom" SHORT @ S+c1−2c2
  "top"    SHORT @ S+c1−2c2−net_debit
"""
from datetime import timedelta, datetime, time as dtime

from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps
from qib_trader.strategies.uturn_bsync_debit_put_spread import _build_sync_timestamps_top
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing


class UTurnBSync_Ratio(Strategy):
    """
    正股在 UTurnBottom 开仓；Ratio Call Spread 可选在 Bottom 或 Top 开仓。
    持有至到期，无滚动/换仓机制。
    """

    options_mode:    bool  = True
    initial_capital: float = 100_000.0

    sync_signals:    list  = []
    start_time:      str   = "2024-01-01 00:00:00"
    end_time:        str   = "2026-06-01 00:00:00"

    uturn_btm_level:    int   = 1      # 1=SimpleUBtm 2=DbSimpleBtm 3=MixDbSimpleBtm
    multi_uturn_window: int   = 50

    # ── Ratio 开仓时机 ────────────────────────────────────────────────────────
    ratio_entry_signal: str  = "bottom"  # "bottom" 或 "top"
    uturn_top_level:    int  = 1         # UTurnTop 等级（ratio_entry_signal=="top" 时生效）
    top_uturn_window:   int  = 50        # Level 2/3 top 信号窗口
    top_sync:           bool = False     # True=用 sync_signals 的交集；False=单品种
    top_code_id:        str  = ""        # 单品种模式下的品种（空串=使用 self.codeId）

    # ── 期权参数 ──────────────────────────────────────────────────────────────
    entry_long:     float = 0.5        # Long call delta（ATM，K1 ≈ S）
    entry_short:    float = 0.25       # Short call delta（OTM，K2 > K1）
    option_days:    int   = 7
    expiry_weekday: int   = -1         # -1=仅跳过周末；0=周一…4=周五
    risk_free_rate: float = 0.04
    strike_round:   float = 100.0
    cooldown_hours: float = 1.0
    max_positions:  int   = 3

    # ── 生命周期 ──────────────────────────────────────────────────────────────

    def on_init(self) -> None:
        self._positions: list[dict] = []
        self._closed_positions: list[dict] = []
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._sync_timestamps: set = set()
        self._top_timestamps:  set = set()

    def on_start(self) -> None:
        self.active = True
        if not self.sync_signals:
            self.logger.warning("sync_signals 为空，策略永远不会开仓")
            return
        level_names = {1: 'SimpleUBtm', 2: 'DbSimpleBtm', 3: 'MixDbSimpleBtm'}
        self._sync_timestamps = _build_sync_timestamps(
            self.sync_signals, self.interval.value,
            self.start_time, self.end_time,
            level=self.uturn_btm_level, window=self.multi_uturn_window,
        )
        self.logger.info(
            f"UTurnBottom 同步时间戳 {len(self._sync_timestamps)} 个"
            f"（{level_names.get(self.uturn_btm_level, self.uturn_btm_level)}）"
        )
        if self.ratio_entry_signal == "top":
            self._build_top_timestamps()

    def _build_top_timestamps(self) -> None:
        top_level_names = {1: 'SimpleUTop', 2: 'DbSimpleTop', 3: 'MixDbSimpleTop'}
        signals = (self.sync_signals if self.top_sync
                   else [{'code_id': self.top_code_id or self.codeId, 'min_dif_abs': 0.0}])
        self._top_timestamps = _build_sync_timestamps_top(
            signals, self.interval.value,
            self.start_time, self.end_time,
            level=self.uturn_top_level, window=self.top_uturn_window,
        )
        self.logger.info(
            f"UTurnTop 时间戳 {len(self._top_timestamps)} 个"
            f"（{top_level_names.get(self.uturn_top_level, self.uturn_top_level)}"
            f" sync={self.top_sync}）"
        )

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for p in list(self._positions):
            self._close_position(p, self._last_bar, 'force_close')
        self._positions.clear()
        # 清仓后重置 MTM 为 0，避免 equity 曲线在最后一 bar 出现双重计数
        # （on_stop 在 logic() 之后执行，_mtm_pnl_series[last_bar] 此前已记录浮动盈亏，
        #  而 force_close 又在同一 bar 结算实现盈亏，二者相加会造成重复）
        self._record_mtm_pnl(self._last_bar)

    # ── 工具 ──────────────────────────────────────────────────────────────────

    def _call_strike_by_delta(self, S: float, T: float, delta: float, iv: float) -> float:
        return option_pricing.call_strike_by_delta(
            S, T, delta, iv, self.risk_free_rate, self.codeId, self.strike_round)

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
        return datetime.combine(expiry_date, dtime(16, 0))

    def _rem_t(self, bar_dt: datetime, expiry: datetime) -> float:
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    def _curr_call_prices(self, bar: BarData, p: dict) -> tuple[float, float]:
        T  = self._rem_t(bar.datetime, p['expiry'])
        S  = bar.close_price
        c1 = option_pricing.call_price(S, p['k1'], T, self.risk_free_rate, p['iv1'], self.codeId)
        c2 = option_pricing.call_price(S, p['k2'], T, self.risk_free_rate, p['iv2'], self.codeId)
        return c1, c2

    # ── 开仓 Ratio ────────────────────────────────────────────────────────────

    def _open_ratio_spread(self, p: dict, bar: BarData) -> None:
        """在当前 bar 为持仓 p 开仓 Ratio Call Spread，并更新持仓字段。"""
        S      = bar.close_price
        expiry = self._next_expiry(bar.datetime, self.option_days)
        T      = (expiry - bar.datetime).total_seconds() / (365.25 * 24 * 3600)
        iv1    = get_iv(self.codeId, 'call', self.entry_long)
        iv2    = get_iv(self.codeId, 'call', self.entry_short)
        k1     = self._call_strike_by_delta(S, T, self.entry_long,  iv1)
        k2     = self._call_strike_by_delta(S, T, self.entry_short, iv2)
        p1     = option_pricing.call_price(S, k1, T, self.risk_free_rate, iv1, self.codeId)
        p2     = option_pricing.call_price(S, k2, T, self.risk_free_rate, iv2, self.codeId)
        p.update({'k1': k1, 'k2': k2, 'p1': p1, 'p2': p2,
                  'net_debit': p1 - 2.0 * p2, 'iv1': iv1, 'iv2': iv2,
                  'expiry': expiry, 'ratio_open_dt': bar.datetime,
                  'ratio_opened': True})
        self.logger.info(
            f"[{bar.datetime}] 开仓Ratio K1={k1} K2={k2} S={S:.2f} "
            f"p1={p1:.4f} p2={p2:.4f} net_debit={p['net_debit']:.4f} expiry={expiry.date()}"
        )

    # ── 到期 / 平仓 ───────────────────────────────────────────────────────────

    def _exit_price_ratio(self, S: float, p: dict) -> float:
        """
        合成 SHORT 价格，使 pnl = SHORT - LONG 等于实际盈亏。
        bottom 模式 LONG @ S0+net_debit → EXIT 不含 net_debit 项。
        top    模式 LONG @ S0           → EXIT 含 -net_debit 项。
        """
        k1, k2  = p['k1'], p['k2']
        nd_adj  = 0.0 if self.ratio_entry_signal == "bottom" else p['net_debit']
        if S < k1:
            return S - nd_adj
        elif S < k2:
            return 2.0 * S - k1 - nd_adj
        else:
            return 2.0 * k2 - k1 - nd_adj

    def _final_pnl_ratio(self, S: float, p: dict) -> float:
        S0, k1, k2, nd = p['entry_underlying'], p['k1'], p['k2'], p['net_debit']
        if S < k1:
            return (S - S0) - nd
        elif S < k2:
            return (2.0 * S - k1 - S0) - nd
        else:
            return (2.0 * k2 - k1 - S0) - nd

    def _record_position(self, p: dict, bar: BarData,
                         close_reason: str, final_pnl: float) -> None:
        self._closed_positions.append({
            'entry_dt':         p['entry_dt'],
            'entry_underlying': p['entry_underlying'],
            'ratio_open_dt':    p.get('ratio_open_dt'),
            'settlement_dt':    bar.datetime,
            'close_reason':     close_reason,
            'ratio_opened':     p['ratio_opened'],
            'k1':               p.get('k1'),
            'k2':               p.get('k2'),
            'p1':               p.get('p1'),
            'p2':               p.get('p2'),
            'net_debit':        p.get('net_debit', 0.0),
            'final_pnl':        final_pnl,
            'notional':         (p['k2'] - p['k1']) if p['ratio_opened'] else 0.0,
        })

    def _settle_position(self, p: dict, bar: BarData,
                         close_reason: str = 'expiry') -> None:
        """到期结算：Ratio 已开仓用三段公式；未开仓（top模式等待中）纯股票平仓。"""
        S  = bar.close_price
        S0 = p['entry_underlying']
        if not p['ratio_opened']:
            # 期权从未开仓（top 模式在 top 信号前强平）
            final_pnl = S - S0
            self._send_order(Direction.SHORT, S, 1, "MKT")
            self._record_position(p, bar, close_reason, final_pnl)
            self.logger.info(f"[{bar.datetime}] {close_reason}(无Ratio) S={S:.2f} pnl={final_pnl:.2f}")
            return
        final_pnl    = self._final_pnl_ratio(S, p)
        effective_ex = self._exit_price_ratio(S, p)
        self._send_order(Direction.SHORT, effective_ex, 1, "MKT")
        self._record_position(p, bar, close_reason, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] {close_reason} K1={p['k1']} K2={p['k2']} S={S:.2f} "
            f"net_debit={p['net_debit']:.4f} pnl={final_pnl:.2f}"
        )

    def _close_position(self, p: dict, bar: BarData, close_reason: str) -> None:
        """提前平仓（MTM）：Ratio 已开仓用 BS 估值；未开仓纯股票市价平。"""
        S  = bar.close_price
        S0 = p['entry_underlying']
        if not p['ratio_opened']:
            final_pnl = S - S0
            self._send_order(Direction.SHORT, S, 1, "MKT")
            self._record_position(p, bar, close_reason, final_pnl)
            self.logger.info(f"[{bar.datetime}] {close_reason}(无Ratio) S={S:.2f} pnl={final_pnl:.2f}")
            return
        c1, c2    = self._curr_call_prices(bar, p)
        nd        = p['net_debit']
        total_pnl = (S - S0) + (c1 - p['p1']) - 2.0 * (c2 - p['p2'])
        # effective_exit = S0 + total_pnl (bottom) 或 S0 + total_pnl（top，LONG@S0）
        #   化简后：S + c1 - 2c2 (bottom) / S + c1 - 2c2 - nd (top)
        nd_adj       = 0.0 if self.ratio_entry_signal == "bottom" else nd
        effective_ex = S + c1 - 2.0 * c2 - nd_adj
        self._send_order(Direction.SHORT, effective_ex, 1, "MKT")
        self._record_position(p, bar, close_reason, total_pnl)
        self.logger.info(
            f"[{bar.datetime}] {close_reason} K1={p['k1']} K2={p['k2']} "
            f"S={S:.2f} c1={c1:.4f} c2={c2:.4f} pnl={total_pnl:.2f}"
        )

    # ── 到期检查 ──────────────────────────────────────────────────────────────

    def _check_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in self._positions:
            if p['ratio_opened'] and bar.datetime >= p['expiry']:
                self._settle_position(p, bar, 'expiry')
            else:
                remaining.append(p)
        self._positions = remaining

    # ── Ratio 开仓检查（top 模式）──────────────────────────────────────────────

    def _check_ratio_open(self, bar: BarData) -> None:
        """top 模式：遇到 UTurnTop 信号时，为所有尚未开仓 Ratio 的持仓开仓。"""
        if self.ratio_entry_signal != "top":
            return
        if bar.datetime not in self._top_timestamps:
            return
        for p in self._positions:
            if not p['ratio_opened']:
                self._open_ratio_spread(p, bar)

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        """MTM：Ratio 已开 → (S−S0)+(c1−p1)−2(c2−p2)；未开 → (S−S0)。"""
        if not self._positions:
            return 0.0
        S, total = bar.close_price, 0.0
        for p in self._positions:
            if p['ratio_opened']:
                c1, c2 = self._curr_call_prices(bar, p)
                total += (S - p['entry_underlying']) + (c1 - p['p1']) - 2.0 * (c2 - p['p2'])
            else:
                total += S - p['entry_underlying']
        return total

    # ── 开仓信号 ──────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if bar.datetime not in self._sync_timestamps:
            return False
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        if self.max_positions > 0 and len(self._positions) >= self.max_positions:
            return False
        return True

    def _check_entry(self, bar: BarData) -> None:
        """
        UTurnBottom 开仓：
          bottom 模式 → 正股 + Ratio 同时开仓，LONG @ S0+net_debit
          top    模式 → 仅开正股，LONG @ S0；Ratio 等 UTurnTop
        """
        if not self._entry_signal(bar):
            return
        S = bar.close_price
        self._last_entry_dt = bar.datetime

        if self.ratio_entry_signal == "bottom":
            p: dict = {'entry_dt': bar.datetime, 'entry_underlying': S,
                       'ratio_opened': False, 'ratio_open_dt': None,
                       'k1': None, 'k2': None, 'p1': None, 'p2': None,
                       'net_debit': None, 'iv1': None, 'iv2': None, 'expiry': None}
            self._open_ratio_spread(p, bar)   # 设置 k1/k2/p1/p2/net_debit/expiry
            self._positions.append(p)
            net_cost = S + p['net_debit']
            self._send_order(Direction.LONG, net_cost, 1, "MKT")
            self.logger.info(f"[{bar.datetime}] 开仓(bottom) net_cost={net_cost:.2f}")
        else:
            # top 模式：仅记录正股，等 UTurnTop 再开 Ratio
            p = {'entry_dt': bar.datetime, 'entry_underlying': S,
                 'ratio_opened': False, 'ratio_open_dt': None,
                 'k1': None, 'k2': None, 'p1': None, 'p2': None,
                 'net_debit': None, 'iv1': None, 'iv2': None, 'expiry': None}
            self._positions.append(p)
            self._send_order(Direction.LONG, S, 1, "MKT")
            self.logger.info(f"[{bar.datetime}] 开仓正股(top模式等待UTurnTop) S={S:.2f}")

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_ratio_open(bar)
        self._check_expiry(bar)
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
