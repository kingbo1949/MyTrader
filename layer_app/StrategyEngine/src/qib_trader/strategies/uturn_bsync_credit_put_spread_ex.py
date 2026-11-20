"""
UTurnBSync_CreditPutSpreadEx 策略（两阶段自动转仓）

Phase 1 — Credit Put Spread（同 UTurnBSync_CreditPutSpread）：
  开仓：UTurnBottom 同步信号 → 卖出 OTM Put K1 + 买入更 OTM Put K2
  到期结算：
    S ≥ K1      → 两腿虚值作废，收取全部净权利金，lifecycle 结束  ✓
    K2 ≤ S < K1 → 短腿行权（以 K1 买入正股），长腿虚值          → 转入 Phase 2
    S < K2      → 两腿均行权，最大亏损 = net_credit-(K1-K2)，lifecycle 结束 ✗
  中途平仓（早止盈 / 价格进度）：lifecycle 结束

Phase 2 — Covered Call + 自动滚仓（逻辑同 UTurnBSync_CoveredCallEx）：
  进入条件：Phase 1 K2 ≤ S < K1 到期行权
  有效正股成本：S0 = K1
  cc_spread_credit = net_credit（长腿虚值，无额外收益）
  逐步卖出 OTM Call → call 虚值到期时保留正股并开新 call（roll 参数独立）
  直到：行权/cc_early_profit/max_roll_count

合成订单设计（lifecycle 整体配对，无 FIFO 错配）：
  开仓  → SHORT @ net_credit
  平仓  → LONG  @ (net_credit - final_pnl)
  验证: SHORT - LONG = final_pnl ✓

final_pnl（Phase 2 最终平仓）：
  = (stock_exit - S0) + cc_spread_credit + prev_premiums + (call_prem - call_close)
"""
from datetime import timedelta, datetime, time as dtime

from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies._cc_dip_mixin import CoveredCallDipMixin
from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps, _load_bars
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing
from qib_trader.live.intent import emit_intent


class UTurnBSync_CreditPutSpreadEx(CoveredCallDipMixin, Strategy):
    """Credit Put Spread → 行权后自动转 Covered Call 并滚仓。"""

    options_mode:    bool  = True
    initial_capital: float = 100_000.0

    sync_signals:       list  = []
    start_time:         str   = "2024-01-01 00:00:00"
    end_time:           str   = "2026-06-01 00:00:00"
    uturn_btm_level:    int   = 1
    multi_uturn_window: int   = 50
    min_dif_abs:        float = 0.0

    # ── Phase 1: 价差开仓参数 ──────────────────────────────────────────────────
    entry_short:    float = 0.4    # 短腿 |put delta|（K1，较高行权价）
    entry_long:     float = 0.1    # 长腿 |put delta|（K2，保护腿）
    option_days:    int   = 7      # 价差 DTE
    expiry_weekday: int   = -1
    min_net_credit: float = 0.0    # 最低净权利金（0=不限制）

    # ── Phase 1: 价差平仓参数 ──────────────────────────────────────────────────
    price_check_hours:       float = 0.0   # 价格进度检查小时数（0=不启用）
    min_price_gain_ratio:    float = 0.0   # 检查时要求最低涨幅（0=不限制）
    spread_early_profit_ratio: float = 0.0 # 价差利润 ≥ ratio×net_credit 时提前止盈
    spread_early_time_ratio:   float = 0.0 # 仅在已用时间 < ratio 时触发（0=不启用）

    # ── Phase 2: 滚仓 call 参数（独立） ───────────────────────────────────────
    roll_entry_short:    float = 0.3   # 滚仓 call delta
    roll_option_days:    int   = 14    # 滚仓 call DTE
    roll_expiry_weekday: int   = -1
    max_roll_count:      int   = 0     # 0=不限制

    # ── Phase 2: 无滑点成交可行性检查（可选） ────────────────────────────────
    # 开/平仓信号 bar 之后，正股需在 fill_check_bars 根 bar 内达到能让期权价格变动
    # fill_check_spread 的价位（按信号 bar 的 delta 折算），否则本次开平仓不成交，
    # 成交价始终等于信号bar理论价（不因此变好或变差）。
    fill_check_bars:   int   = 0    # 0=不启用（保持现有 close_price 瞬时成交行为）
    fill_check_spread: float = 1.0  # 假设的期权 bid/ask 宽度（美元）

    # ── Phase 2: profit_roll 延迟重开（可选） ────────────────────────────────
    # False（默认）：买回旧call后立即在当前价位卖出新call（原有行为，原子操作）。
    # True：买回旧call后不立即重开，转为 dip_waiting 裸持正股，复用 dip_reopen_level/
    # dip_reopen_sync/dip_reopen_code_id/dip_reopen_window 这套 UTurnTop 信号和
    # _check_dip_reopen 的重开逻辑（含 dip_roll_option_days），等到信号出现才重新卖出
    # 新call。需要 dip_reopen_level > 0 才会生效，否则退化为一直裸持到强平/其他事件。
    profit_roll_wait_reopen: bool = False

    # ── Phase 2: CC 平仓参数 ───────────────────────────────────────────────────
    cc_roll_profit_ratio:  float = 0.0  # short call 权利金实现率达到此比例时买回并在新价位展期（0=不启用）
    cc_early_profit_ratio: float = 0.0  # total_pnl ≥ ratio×max_profit 且早期触发
    cc_early_time_ratio:   float = 0.0

    # ── 下跌回调展期 ──────────────────────────────────────────────────────────
    # 滚动态（CC Phase）：UTurnBtm 出现时买回 short call，等 UTurnTop 后重新卖出。
    # 价差态（Spread Phase，需 dip_apply_to_spread=True）：
    #   UTurnBtm 出现时卖出 long put 腿，等 UTurnTop 后以相同到期日重新买入。
    #   此间 net_credit 实时修正，_spread_curr_price 自动退化为仅含 short put。
    dip_close_level:    int  = 0     # UTurnBtm 等级（0=不启用此功能）
                                     # 1=SimpleUBtm 2=DbSimpleBtm 3=MixDbSimpleBtm
    dip_close_sync:     bool = True   # True=同步信号；False=使用 dip_close_code_id 单品种
    dip_close_code_id:  str  = ""    # 单品种模式下的品种（空串=使用交易标的本身）
    dip_close_window:   int  = 50    # Level 2/3 需要的历史 bar 窗口大小

    dip_reopen_level:   int  = 0     # UTurnTop 等级（0=立即展期，不等信号）
                                     # 1=SimpleUTop 2=DbSimpleTop 3=MixDbSimpleTop
    dip_reopen_sync:    bool = True   # True=同步信号；False=使用 dip_reopen_code_id 单品种
    dip_reopen_code_id: str  = ""    # 单品种模式下的品种（空串=使用交易标的本身）
    dip_reopen_window:  int  = 50    # Level 2/3 需要的历史 bar 窗口大小
    dip_roll_option_days: int = 0   # 滚动态 dip 重建 call 的到期天数（0=使用 roll_option_days）

    dip_apply_to_spread: bool = False  # True=价差阶段也适用（卖出/重开 long put）
                                       # False=仅滚动/CC 阶段适用
    dip_reopen_allow_early_profit: bool = True
    # False：通过 dip_reopen 机制开出的 call（p['from_dip_reopen']=True）不参与
    # cc_early_profit 提前整体平仓判断——避免裸持期间已经积累的浮盈，让刚重开的
    # call 在很短时间内（甚至同一根bar）就被连带平仓、权利金空转一次。此时这类
    # 仓位只能通过 call 自然到期（expiry_assignment）或 force_close 结束
    # （max_roll_exceeded 理论上也算一条路径，但本项目 max_roll_count 恒为0，
    # 实际不可达，未做额外处理）。

    # ── 共用参数 ───────────────────────────────────────────────────────────────
    min_vix:        float = 0.0   # 开仓时 VIX 须 ≥ 此值（0=不限制）
    vix_code_id:    str   = "NQ"  # VIX 来源品种（ES/NQ 均可，index_close 即为 VIX；空串=用交易标的自身）
    risk_free_rate: float = 0.04
    strike_round:   float = 100.0
    cooldown_hours: float = 1.0
    max_positions:  int   = 3

    # ── 生命周期 ──────────────────────────────────────────────────────────────

    def on_init(self) -> None:
        self._positions: list[dict] = []
        self._closed_positions: list[dict] = []
        self._roll_log: list[dict] = []
        self._position_seq = 0
        self._fill_check_warned = False
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._sync_timestamps: set       = set()
        self._dip_close_timestamps: set  = set()
        self._dip_reopen_timestamps: set = set()

    def on_start(self) -> None:
        self.active = True
        if not self.sync_signals:
            self.logger.warning("sync_signals 为空，策略永远不会开仓")
            return
        self._sync_timestamps = _build_sync_timestamps(
            self.sync_signals, self.interval.value,
            self.start_time, self.end_time,
            level=self.uturn_btm_level, window=self.multi_uturn_window,
        )
        if self.dip_close_level > 0:
            self._dip_close_timestamps = self._build_btm_timestamps(
                self.dip_close_sync, self.dip_close_code_id,
                self.dip_close_level, self.dip_close_window)
            self.logger.info(
                f"下跌平仓时间戳 {len(self._dip_close_timestamps)} 个"
                f"（等级{self.dip_close_level} sync={self.dip_close_sync}）"
            )
        # profit_roll_wait_reopen 复用同一套 UTurnTop 时间戳，即使 dip_close_level=0
        # （不启用下跌避险）也需要单独构建
        if self.dip_reopen_level > 0 and (self.dip_close_level > 0 or self.profit_roll_wait_reopen):
            self._dip_reopen_timestamps = self._build_top_timestamps(
                self.dip_reopen_sync, self.dip_reopen_code_id,
                self.dip_reopen_level, self.dip_reopen_window)
            self.logger.info(
                f"反弹展期时间戳 {len(self._dip_reopen_timestamps)} 个"
                f"（等级{self.dip_reopen_level} sync={self.dip_reopen_sync}）"
            )
        # 预构建 VIX 查找表（datetime → vix）
        self._vix_map: dict = {}
        if self.min_vix > 0 and self.vix_code_id:
            vix_bars = _load_bars(self.vix_code_id, self.interval.value,
                                  self.start_time, self.end_time)
            self._vix_map = {
                b.datetime: b.index_close
                for b in vix_bars if b.index_valid and b.index_close > 0
            }
            self.logger.info(f"VIX 查找表构建完毕: {self.vix_code_id} {len(self._vix_map)} 条记录")

    def on_stop(self) -> None:
        self.active = False
        if self._last_bar is None:
            return
        for p in list(self._positions):
            self._force_close(p, self._last_bar)
        self._positions.clear()
        # 清仓后重置 MTM 为 0，避免 equity 曲线在最后一 bar 出现双重计数
        # （on_stop 在 logic() 之后执行，_mtm_pnl_series[last_bar] 此前已记录浮动盈亏，
        #  而 force_close 又在同一 bar 结算实现盈亏，二者相加会造成重复）
        self._record_mtm_pnl(self._last_bar)

    # ── 通用工具 ──────────────────────────────────────────────────────────────

    def _next_expiry(self, bar_dt: datetime, min_days: int, weekday: int) -> datetime:
        target = bar_dt + timedelta(days=min_days)
        if weekday == -1 or min_days == 1:
            extra = {5: 2, 6: 1}.get(target.weekday(), 0)
            expiry_date = (target + timedelta(days=extra)).date()
        else:
            days_ahead = (weekday - target.weekday()) % 7
            expiry_date = (target + timedelta(days=days_ahead)).date()
        return datetime.combine(expiry_date, dtime(16, 0))

    def _rem_t(self, bar_dt: datetime, expiry: datetime) -> float:
        return max((expiry - bar_dt).total_seconds() / (365.25 * 24 * 3600), 0.0)

    def _force_close(self, p: dict, bar: BarData) -> None:
        if p['phase'] == 'spread':
            # _spread_curr_price 在 spread_dip_waiting 时自动退化为仅含 short put
            final_pnl = p['net_credit'] - self._spread_curr_price(bar, p)
            self._finalize_spread(p, bar, 'force_close', final_pnl)
        else:
            self._cc_close_all(p, bar, 'force_close')

    # ── Phase 1: 价差工具 ─────────────────────────────────────────────────────

    def _spread_curr_price(self, bar: BarData, p: dict, S: float = None) -> float:
        """当前价差回购成本（curr_p1 - curr_p2）。
        spread_dip_waiting=True 时 long leg 已卖出，只返回 short put 价值。"""
        if S is None:
            S = bar.close_price
        T     = self._rem_t(bar.datetime, p['spread_expiry'])
        curr1 = option_pricing.put_price(S, p['k1'], T, self.risk_free_rate, p['iv_short'], self.codeId)
        if p.get('spread_dip_waiting', False):
            return curr1
        curr2 = option_pricing.put_price(S, p['k2'], T, self.risk_free_rate, p['iv_long'],  self.codeId)
        return curr1 - curr2

    def _finalize_spread(self, p: dict, bar: BarData,
                         close_reason: str, final_pnl: float) -> None:
        """结束 Phase 1 lifecycle，记录并发出合成 LONG 订单。"""
        self._send_order(Direction.LONG, p['net_credit'] - final_pnl, 1, "MKT")
        self._closed_positions.append({
            'position_id': p['position_id'], 'phase': 'spread',
            'entry_dt': p['entry_dt'],        'settlement_dt': bar.datetime,
            'entry_underlying': p['entry_underlying'],
            'k1': p['k1'],                    'k2': p['k2'],
            'net_credit': p['net_credit'],    'close_reason': close_reason,
            'final_pnl': final_pnl,
        })
        self.logger.info(
            f"[{bar.datetime}] Spread平仓 {close_reason} "
            f"K1={p['k1']} K2={p['k2']} pnl={final_pnl:.4f}"
        )
        emit_intent(self, 'spread_close', p, bar, right='put', leg_name='K1_short',
                    old_k=p['k1'], old_prem=p.get('short_prem'), close_reason=close_reason)
        if not p.get('spread_dip_waiting', False):
            emit_intent(self, 'spread_close', p, bar, right='put', leg_name='K2_long',
                        old_k=p['k2'], old_prem=p.get('long_prem'), close_reason=close_reason)

    # ── Phase 1: 到期结算 & 转仓 ──────────────────────────────────────────────

    def _settle_spread_expiry(self, p: dict, bar: BarData) -> bool:
        """到期结算。True=转入 Phase 2（position 保留），False=lifecycle 结束。
        spread_dip_waiting=True 时无 long leg，仅区分 S>=K1 和 S<K1。"""
        S, K1, K2 = bar.close_price, p['k1'], p['k2']
        if S >= K1:
            self._finalize_spread(p, bar, 'expiry_otm', p['net_credit'])
            return False
        if not p.get('spread_dip_waiting', False) and S < K2:
            # 有 long leg 且两腿均行权：最大亏损封顶
            final_pnl = p['net_credit'] - (K1 - K2)
            self._finalize_spread(p, bar, 'expiry_max_loss', final_pnl)
            return False
        # K2 ≤ S < K1（含 spread_dip_waiting 时 S < K1 全部转仓）
        self._transition_to_cc(p, bar)
        return True

    def _transition_to_cc(self, p: dict, bar: BarData) -> None:
        """将 spread position 就地改写为 covered_call phase。
        进入条件：K2 ≤ S < K1，长腿虚值作废，cc_spread_credit = net_credit。
        """
        S = bar.close_price
        p['phase']           = 'covered_call'
        p['S0']              = p['k1']
        p['cc_spread_credit'] = p['net_credit']   # 长腿虚值，无额外收益
        p['prev_premiums']   = 0.0
        p['roll_count']      = 0
        p['roll_history']    = []
        self._stock_leg_open(p, bar)
        flds = self._open_call_checked(bar, S, self.roll_entry_short,
                                       self.roll_option_days, self.roll_expiry_weekday)
        if flds is None:
            p['call_k'] = p['call_prem'] = p['call_iv'] = 0.0
            p['dip_waiting'] = True
            p['call_open_pending'] = True
            p['call_open_pending_days'] = self.roll_option_days
            p['call_open_pending_event'] = 'cc_open'
            self.logger.info(
                f"[{bar.datetime}] 转仓→CC 首次开call 未通过成交检查，转为逐bar重试: "
                f"S={S:.2f} K1={p['S0']:.2f} K2={p['k2']:.2f} cc_spread_credit={p['cc_spread_credit']:.4f}"
            )
            return
        p.update(flds)
        p['from_dip_reopen'] = False
        self._append_roll_history(p, bar, 'cc_open',
                                  new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                  new_expiry=p['call_expiry'])
        self.logger.info(
            f"[{bar.datetime}] 转仓→CC: S={S:.2f} K1={p['S0']:.2f} K2={p['k2']:.2f} "
            f"cc_spread_credit={p['cc_spread_credit']:.4f} "
            f"call_k={p['call_k']} exp={p['call_expiry'].date()}"
        )

    # ── "正股腿"可覆盖钩子（供 PMCC 等子类替换持有正股的方式用）───────────────────
    # 基类里这几个钩子在数学上跟直接持有正股完全等价，不改变 Ex 自身行为；
    # 子类只需要重写这几个方法就能把"正股"换成别的东西（比如深度实值远期call）。

    def _stock_leg_open(self, p: dict, bar: BarData) -> None:
        """接货那一刻调用（在 p['S0'] 设置之后）。基类持有的就是正股本身，无需额外动作。"""
        pass

    def _stock_leg_value(self, bar: BarData, p: dict, S: float) -> float:
        """"正股腿"在标的价格为 S 时对总盈亏的贡献。基类即线性 S - S0。"""
        return S - p['S0']

    def _stock_leg_close(self, p: dict, bar: BarData, close_reason: str, stock_exit: float) -> None:
        """CC phase 结束（不管什么原因）时调用，用于额外记录/上报"正股腿"的平仓意图。
        基类里卖正股这件事已经完全由 _cc_finalize 的主流程覆盖，无需额外动作。"""
        pass

    def _stock_leg_maintain(self, p: dict, bar: BarData) -> None:
        """covered_call 阶段每根bar对"正股腿"做一次维护性检查（比如子类的期权型
        替代品需要在自己到期前主动展期）。基类持有的就是正股本身，没有到期日
        这回事，无需任何动作。"""
        pass

    def _extra_record_fields(self, p: dict) -> dict:
        """_cc_record 写入 _closed_positions 时合并的额外字段，供子类记录自己的持仓明细。"""
        return {}

    def _check_stock_leg_maintenance(self, bar: BarData) -> None:
        """对所有 covered_call 阶段的仓位调用一次 _stock_leg_maintain。跟
        dip_waiting 状态无关——"正股腿"（或其替代品）的生命周期不依赖于当前
        是否持有 short call。"""
        for p in self._positions:
            if p['phase'] == 'covered_call':
                self._stock_leg_maintain(p, bar)

    def _check_spread_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in list(self._positions):
            if p['phase'] == 'spread' and bar.datetime >= p['spread_expiry']:
                if self._settle_spread_expiry(p, bar):
                    remaining.append(p)   # 已转仓，保留
            else:
                remaining.append(p)
        self._positions = remaining

    # ── Phase 1: 价格进度 / 早止盈 ───────────────────────────────────────────

    def _check_spread_price_progress(self, bar: BarData) -> None:
        if self.price_check_hours <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread' or p['price_checked']:
                remaining.append(p)
                continue
            elapsed_h = (bar.datetime - p['entry_dt']).total_seconds() / 3600
            if elapsed_h < self.price_check_hours:
                remaining.append(p)
                continue
            p['price_checked'] = True
            gain = (bar.close_price - p['entry_underlying']) / p['entry_underlying']
            if gain < self.min_price_gain_ratio:
                final_pnl = p['net_credit'] - self._spread_curr_price(bar, p)
                self._finalize_spread(p, bar, 'price_progress_stop', final_pnl)
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_spread_early_profit(self, bar: BarData) -> None:
        if self.spread_early_profit_ratio <= 0 or self.spread_early_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'spread':
                remaining.append(p)
                continue
            total_s   = (p['spread_expiry'] - p['entry_dt']).total_seconds()
            elapsed_s = (bar.datetime - p['entry_dt']).total_seconds()
            if total_s <= 0 or elapsed_s / total_s >= self.spread_early_time_ratio:
                remaining.append(p)
                continue
            curr_spread = self._spread_curr_price(bar, p)
            curr_profit = p['net_credit'] - curr_spread
            if p['net_credit'] > 0 and curr_profit >= self.spread_early_profit_ratio * p['net_credit']:
                self._finalize_spread(p, bar, 'early_profit', curr_profit)
            else:
                remaining.append(p)
        self._positions = remaining

    # ── Phase 2: CC 工具 ──────────────────────────────────────────────────────

    def _make_call_fields(self, bar: BarData, S: float,
                          delta: float, opt_days: int, exp_weekday: int) -> dict:
        expiry = self._next_expiry(bar.datetime, opt_days, exp_weekday)
        T      = self._rem_t(bar.datetime, expiry)
        iv     = get_iv(self.codeId, 'call', delta)
        k      = option_pricing.call_strike_by_delta(
            S, T, delta, iv, self.risk_free_rate, self.codeId, self.strike_round)
        prem   = option_pricing.call_price(S, k, T, self.risk_free_rate, iv, self.codeId)
        return {'call_k': k, 'call_prem': prem, 'call_iv': iv,
                'call_expiry': expiry, 'call_entry_dt': bar.datetime}

    def _cc_max_profit(self, p: dict, bar: BarData) -> float:
        """假设标的价格恰好在 call_k（short call 满值行权）时的理论最大总盈亏。"""
        return (self._stock_leg_value(bar, p, p['call_k'])
                + p['cc_spread_credit'] + p['prev_premiums'] + p['call_prem'])

    def _cc_total_pnl(self, bar: BarData, p: dict, S: float = None):
        """返回 (total_pnl, curr_call_price)。"""
        if S is None:
            S = bar.close_price
        T        = self._rem_t(bar.datetime, p['call_expiry'])
        curr_call = option_pricing.call_price(
            S, p['call_k'], T, self.risk_free_rate, p['call_iv'], self.codeId)
        pnl = (self._stock_leg_value(bar, p, S)
               + p['cc_spread_credit'] + p['prev_premiums']
               + (p['call_prem'] - curr_call))
        return pnl, curr_call

    def _cc_elapsed_ratio(self, bar: BarData, p: dict) -> float:
        total   = (p['call_expiry'] - p['call_entry_dt']).total_seconds()
        elapsed = (bar.datetime - p['call_entry_dt']).total_seconds()
        return elapsed / total if total > 0 else 1.0

    def _cc_record(self, p: dict, bar: BarData, close_reason: str,
                   stock_exit: float, call_close: float, final_pnl: float) -> None:
        """记录 CC phase 到 _closed_positions。"""
        record = {
            'position_id': p['position_id'], 'phase': 'covered_call',
            'close_reason': close_reason,
            'entry_dt': p['entry_dt'],        'settlement_dt': bar.datetime,
            'entry_underlying': p['entry_underlying'],
            'k1': p['k1'],                    'k2': p['k2'],
            'net_credit': p['net_credit'],    'S0': p['S0'],
            'cc_spread_credit': p['cc_spread_credit'],
            'roll_count': p['roll_count'],    'prev_premiums': p['prev_premiums'],
            'call_k': p['call_k'],            'call_prem': p['call_prem'],
            'call_close': call_close,         'stock_exit': stock_exit,
            'final_pnl': final_pnl,
        }
        record.update(self._extra_record_fields(p))
        self._closed_positions.append(record)

    def _cc_finalize(self, p: dict, bar: BarData, close_reason: str,
                     stock_exit: float, call_close: float) -> None:
        """平仓 CC phase，发出合成 LONG 订单（与 entry SHORT 配对）。"""
        final_pnl = (self._stock_leg_value(bar, p, stock_exit)
                     + p['cc_spread_credit'] + p['prev_premiums']
                     + (p['call_prem'] - call_close))
        self._send_order(Direction.LONG, p['net_credit'] - final_pnl, 1, "MKT")
        self._cc_record(p, bar, close_reason, stock_exit, call_close, final_pnl)
        self.logger.info(
            f"[{bar.datetime}] CC平仓 {close_reason} rolls={p['roll_count']} "
            f"S0={p['S0']:.2f} exit={stock_exit:.2f} "
            f"spread_cr={p['cc_spread_credit']:.4f} pnl={final_pnl:.4f}"
        )
        if p['call_k']:   # dip_waiting 时已经没有持有的call，不生成CLOSE意图
            emit_intent(self, 'cc_close', p, bar, right='call',
                        old_k=p['call_k'], old_prem=p['call_prem'], close_price=call_close,
                        close_reason=close_reason)
        self._stock_leg_close(p, bar, close_reason, stock_exit)

    def _cc_close_all(self, p: dict, bar: BarData, close_reason: str,
                      stock_price: float = None) -> None:
        """平仓 CC position（dip_waiting 时无 short call，call_close=0）。"""
        S = stock_price if stock_price is not None else bar.close_price
        if p.get('dip_waiting', False):
            self._cc_finalize(p, bar, close_reason, S, 0.0)
        else:
            _, curr_call = self._cc_total_pnl(bar, p, S)
            self._cc_finalize(p, bar, close_reason, S, curr_call)

    # ── Spread Phase 下跌处理（覆盖 CoveredCallDipMixin 的 no-op 钩子）────────

    def _dip_close_spread_leg(self, p: dict, bar: BarData) -> None:
        """卖出 long put 腿：将卖出所得并入 net_credit，进入 spread_dip_waiting。"""
        S        = bar.close_price
        T        = self._rem_t(bar.datetime, p['spread_expiry'])
        curr_put = option_pricing.put_price(S, p['k2'], T,
                                            self.risk_free_rate, p['iv_long'], self.codeId)
        p['net_credit']          += curr_put   # 卖出 long put 所得并入信用
        p['spread_dip_waiting']   = True
        self.logger.info(
            f"[{bar.datetime}] 下跌卖long put: S={S:.2f} K2={p['k2']:.2f} "
            f"curr_put={curr_put:.4f} adj_credit={p['net_credit']:.4f}"
        )
        emit_intent(self, 'spread_dip_close_leg', p, bar, right='put', leg_name='K2_long',
                    old_k=p['k2'], old_prem=p.get('long_prem'), close_price=curr_put)

    def _dip_reopen_spread_leg(self, p: dict, bar: BarData) -> None:
        """重新买入 long put 腿，复用原始 K2 和 IV，到期日与 short put K1 一致。"""
        S      = bar.close_price
        T      = self._rem_t(bar.datetime, p['spread_expiry'])
        p2_new = option_pricing.put_price(S, p['k2'], T,
                                          self.risk_free_rate, p['iv_long'], self.codeId)
        p['net_credit']        -= p2_new    # 买入 long put 的成本
        p['long_prem']          = p2_new
        p['spread_dip_waiting'] = False
        self.logger.info(
            f"[{bar.datetime}] 反弹买long put: S={S:.2f} K2={p['k2']:.2f}(原始) "
            f"prem={p2_new:.4f} exp={p['spread_expiry'].date()} adj_credit={p['net_credit']:.4f}"
        )
        # K2 是复用原有行权价（不是按delta重新算的），target_delta 留空，
        # option_match.py 直接吸附 theo_strike=K2 到最接近的真实行权价即可
        emit_intent(self, 'spread_dip_reopen_leg', p, bar, right='put', leg_name='K2_long',
                    new_k=p['k2'], new_prem=p2_new, new_expiry=p['spread_expiry'],
                    target_days=(p['spread_expiry'] - bar.datetime).days)

    # ── Phase 2: CC 到期结算 & 滚仓 ───────────────────────────────────────────

    def _cc_settle_expiry(self, p: dict, bar: BarData) -> bool:
        """True=滚仓（position 保留），False=lifecycle 结束。"""
        S, K = bar.close_price, p['call_k']
        if S >= K:
            self._cc_finalize(p, bar, 'expiry_assignment', K, 0.0)
            return False
        # call 虚值：锁定已实现权利金，立即展期
        p['prev_premiums'] += p['call_prem']
        new_roll = p['roll_count'] + 1
        if self.max_roll_count > 0 and new_roll > self.max_roll_count:
            self._cc_finalize(p, bar, 'max_roll_exceeded', S, 0.0)
            return False
        old_k, old_prem = p['call_k'], p['call_prem']
        flds = self._open_call_checked(bar, S, self.roll_entry_short,
                                       self.roll_option_days, self.roll_expiry_weekday)
        if flds is None:
            p['call_k'] = p['call_prem'] = p['call_iv'] = 0.0
            p['dip_waiting'] = True
            p['call_open_pending'] = True
            p['call_open_pending_days'] = self.roll_option_days
            p['call_open_pending_event'] = 'expiry_otm_roll'
            self.logger.info(f"[{bar.datetime}] CC到期滚仓 未通过成交检查，转为逐bar重试")
            return True
        p['roll_count'] = new_roll
        p.update(flds)
        p['from_dip_reopen'] = False
        self._append_roll_history(p, bar, 'expiry_otm_roll',
                                  old_call_k=old_k, old_call_prem=old_prem, buyback_price=0.0,
                                  new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                  new_expiry=p['call_expiry'])
        self.logger.info(
            f"[{bar.datetime}] CC滚仓#{p['roll_count']} "
            f"call_k={p['call_k']} prem={p['call_prem']:.4f} "
            f"prev={p['prev_premiums']:.4f} exp={p['call_expiry'].date()}"
        )
        return True

    def _check_cc_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in list(self._positions):
            if (p['phase'] == 'covered_call'
                    and not p.get('dip_waiting', False)
                    and bar.datetime >= p['call_expiry']):
                if self._cc_settle_expiry(p, bar):
                    remaining.append(p)
            else:
                remaining.append(p)
        self._positions = remaining

    # ── Phase 2: CC 权利金展期（下跌后在新价位重开 short call）──────────────────

    def _check_cc_roll_profit(self, bar: BarData) -> None:
        """short call 权利金实现率 ≥ cc_roll_profit_ratio 时买回并在新价位展期。
        典型场景：下跌后 OTM call 几乎归零，买回后在新价位（更 OTM）重开。
        本函数的两条腿（买回旧call、profit_roll_wait_reopen=False时的原子卖新call）
        不受 fill_check_bars/fill_check_spread 无滑点检查约束，始终按信号bar理论价
        瞬时成交——触发条件本身（权利金已衰减到位）已经反映了"当前价位可接受"，
        不再额外叠加成交可行性门槛。profit_roll_wait_reopen=True 时后续由
        _check_dip_reopen 负责重开，那条路径仍受检查约束。
        """
        if self.cc_roll_profit_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if p['phase'] != 'covered_call' or p.get('dip_waiting', False) or p['call_prem'] <= 0:
                remaining.append(p)
                continue
            S         = bar.close_price
            T         = self._rem_t(bar.datetime, p['call_expiry'])
            curr_call = option_pricing.call_price(
                S, p['call_k'], T, self.risk_free_rate, p['call_iv'], self.codeId)
            realized  = (p['call_prem'] - curr_call) / p['call_prem']
            if realized < self.cc_roll_profit_ratio:
                remaining.append(p)
                continue
            new_roll = p['roll_count'] + 1
            if self.max_roll_count > 0 and new_roll > self.max_roll_count:
                self._cc_finalize(p, bar, 'max_roll_exceeded', S, curr_call)
                continue
            close_price = curr_call   # 买回旧call不做无滑点检查，触发条件本身已反映当前价位可接受
            if self.profit_roll_wait_reopen:
                old_k, old_prem = p['call_k'], p['call_prem']
                p['prev_premiums'] += p['call_prem'] - close_price
                p['roll_count']     = new_roll
                p['call_k'] = p['call_prem'] = p['call_iv'] = 0.0
                p['dip_waiting'] = True
                self._append_roll_history(p, bar, 'profit_roll_close',
                                          old_call_k=old_k, old_call_prem=old_prem,
                                          buyback_price=close_price)
                self.logger.info(
                    f"[{bar.datetime}] CC权利金展期#{new_roll} 买回旧call后不立即重开，"
                    f"等待UTurnTop信号: buyback={close_price:.4f}"
                )
                remaining.append(p)
                continue
            new_flds = self._make_call_fields(bar, S, self.roll_entry_short,
                                              self.roll_option_days, self.roll_expiry_weekday)
            old_k, old_prem = p['call_k'], p['call_prem']
            p['prev_premiums'] += p['call_prem'] - close_price
            p['roll_count']     = new_roll
            p.update(new_flds)
            p['from_dip_reopen'] = False
            self._append_roll_history(p, bar, 'profit_roll',
                                      old_call_k=old_k, old_call_prem=old_prem,
                                      buyback_price=close_price,
                                      new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                      new_expiry=p['call_expiry'])
            self.logger.info(
                f"[{bar.datetime}] CC权利金展期#{new_roll} "
                f"realized={realized:.1%} buyback={close_price:.4f} "
                f"→ call_k={p['call_k']} prem={p['call_prem']:.4f} "
                f"exp={p['call_expiry'].date()}"
            )
            remaining.append(p)
        self._positions = remaining

    # ── Phase 2: CC 早止盈 ────────────────────────────────────────────────────

    def _check_cc_early_profit(self, bar: BarData) -> None:
        if self.cc_early_profit_ratio <= 0 or self.cc_early_time_ratio <= 0:
            return
        remaining = []
        for p in self._positions:
            if (p['phase'] != 'covered_call' or p.get('dip_waiting', False)
                    or (p.get('from_dip_reopen', False) and not self.dip_reopen_allow_early_profit)):
                remaining.append(p)
                continue
            max_p  = self._cc_max_profit(p, bar)
            pnl, _ = self._cc_total_pnl(bar, p)
            if (max_p > 0
                    and self._cc_elapsed_ratio(bar, p) < self.cc_early_time_ratio
                    and pnl >= self.cc_early_profit_ratio * max_p):
                self._cc_close_all(p, bar, 'cc_early_profit')
            else:
                remaining.append(p)
        self._positions = remaining

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        if not self._positions:
            return 0.0
        S, total = bar.close_price, 0.0
        for p in self._positions:
            if p['phase'] == 'spread':
                # _spread_curr_price 在 spread_dip_waiting 时仅含 short put
                total += p['net_credit'] - self._spread_curr_price(bar, p, S)
            elif p.get('dip_waiting', False):
                # CC 等待态：无 short call，MTM = 正股腿浮盈 + 已累计权利金 + cc_spread_credit
                total += (self._stock_leg_value(bar, p, S)
                         + p['cc_spread_credit'] + p['prev_premiums'])
            else:
                pnl, _ = self._cc_total_pnl(bar, p)
                total  += pnl
        return total

    # ── 开仓 ──────────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if bar.datetime not in self._sync_timestamps:
            return False
        if self.min_dif_abs > 0 and abs(bar.dif) < self.min_dif_abs:
            return False
        if self.min_vix > 0:
            if self.vix_code_id:
                vix = self._vix_map.get(bar.datetime, 0.0)
            else:
                vix = bar.index_close if bar.index_valid else 0.0
            if vix < self.min_vix:
                return False
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        return not (self.max_positions > 0 and len(self._positions) >= self.max_positions)

    def _execute_spread_entry(self, bar: BarData) -> None:
        expiry   = self._next_expiry(bar.datetime, self.option_days, self.expiry_weekday)
        T        = self._rem_t(bar.datetime, expiry)
        iv_short = get_iv(self.codeId, 'put', self.entry_short)
        iv_long  = get_iv(self.codeId, 'put', self.entry_long)
        k1 = option_pricing.put_strike_by_delta(
            bar.close_price, T, self.entry_short, iv_short, self.risk_free_rate, self.codeId, self.strike_round)
        k2 = option_pricing.put_strike_by_delta(
            bar.close_price, T, self.entry_long, iv_long, self.risk_free_rate, self.codeId, self.strike_round)
        p1  = option_pricing.put_price(bar.close_price, k1, T, self.risk_free_rate, iv_short, self.codeId)
        p2  = option_pricing.put_price(bar.close_price, k2, T, self.risk_free_rate, iv_long,  self.codeId)
        net_credit = p1 - p2
        if self.min_net_credit > 0 and net_credit < self.min_net_credit:
            return
        self._last_entry_dt = bar.datetime
        self._position_seq += 1
        p = {
            'position_id': self._position_seq, 'phase': 'spread',
            'entry_dt': bar.datetime,      'entry_underlying': bar.close_price,
            'k1': k1,                      'k2': k2,
            'short_prem': p1,              'long_prem': p2,
            'net_credit': net_credit,      'spread_expiry': expiry,
            'iv_short': iv_short,          'iv_long': iv_long,
            'price_checked': False,
        }
        self._positions.append(p)
        self._send_order(Direction.SHORT, net_credit, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓 BullPutSpread K1={k1} K2={k2} "
            f"net_credit={net_credit:.4f} exp={expiry.date()}"
        )
        target_days = (expiry - bar.datetime).days
        emit_intent(self, 'spread_entry', p, bar, right='put', leg_name='K1_short',
                    new_k=k1, new_prem=p1, new_expiry=expiry,
                    target_delta=self.entry_short, target_days=target_days,
                    expiry_weekday=self.expiry_weekday)
        emit_intent(self, 'spread_entry', p, bar, right='put', leg_name='K2_long',
                    new_k=k2, new_prem=p2, new_expiry=expiry,
                    target_delta=self.entry_long, target_days=target_days,
                    expiry_weekday=self.expiry_weekday)

    def _check_entry(self, bar: BarData) -> None:
        if self._entry_signal(bar):
            self._execute_spread_entry(bar)

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_spread_expiry(bar)        # Phase 1 到期（可能转仓）
        self._check_stock_leg_maintenance(bar) # 正股腿维护（基类no-op，子类可自行到期展期）
        self._check_cc_expiry(bar)            # Phase 2 call 到期（跳过 dip_waiting）
        self._check_cc_roll_profit(bar)       # Phase 2 权利金展期（跳过 dip_waiting）
        self._check_dip_close(bar)            # UTurnBtm → 卖出 long put / 买回 call
        self._check_dip_reopen(bar)           # UTurnTop → 重买 long put / 重开 call
        self._check_pending_call_open(bar)    # 逐bar重试因未通过成交检查而挂起的开仓
        self._check_spread_price_progress(bar)
        self._check_spread_early_profit(bar)
        self._check_cc_early_profit(bar)      # 跳过 dip_waiting
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
