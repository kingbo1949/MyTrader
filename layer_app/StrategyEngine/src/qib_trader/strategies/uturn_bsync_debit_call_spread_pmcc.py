"""
UTurnBSync_DebitCallSpreadPMCC —— 跟 UTurnBSync_DebitCallSpreadEx 类似，但从
开仓起就是**对角价差**（long call远期200天+short call近期），不再有"Phase1
垂直价差到期后落在某个区间才转仓"这个中间阶段——本质上是让"Phase2那台covered
call滚仓引擎"从第一天就直接跑：

  开仓：UTurnBottom信号（且min_drop_from_prev_close>0时还要求当前价低于昨收
        此比例，例如0.02=须跌破昨收2%，用来过滤"信号出现但跌幅不够深"的情况）
        → 买入远期(long_call_days，默认200天)实值call(K1，entry_long delta)
        + 卖出近期(roll_option_days)虚值call(K2，roll_entry_short delta)，
        K2 > K1。net_cost = long_call_prem - call_prem（买long call花的钱
        减去卖short call收的钱，通常为正）。

  short call到期结算（_check_short_call_expiry）：由settle_itm_via_exercise
  控制S>K2（ITM）到期时的处理方式，用于在"long call永不平仓"（新版本）和
  "行权覆盖两腿一起离场"（老版本）之间切换：
    settle_itm_via_exercise=False（默认，新版本）→ **只平short call这一条腿，
      long call永远不受影响、不会被行权覆盖、不会被平仓**——不管到期时是实值
      还是虚值，统一按"权利金 - 到期时的内在价值"（= call_prem - max(S-K2,0)，
      用_short_call_value在T=0时自然收敛到内在价值这一既有性质计算）banked
      进prev_premiums，转入dip_waiting。long call因此是严格意义上"永不平仓、
      只靠长期展期/独立dip信号滚动"的一条腿，position的lifecycle也不再因
      short call到期而结束——只会在max_roll_count超限（mixin的
      max_roll_exceeded路径）或回测结束（on_stop的force_close）时才真正
      平仓离场。
    settle_itm_via_exercise=True（老版本）→ S>K2时**按当日收盘价把两条腿
      分别按各自理论价"市价平仓"**（直接复用_cc_finalize），而不是行权/
      被指派——long call按bar.close_price算出的BS理论价卖出（保留其剩余
      时间价值，不强制归零），short call按同一时刻的理论价买回（此刻T=0，
      理论价自然收敛为内在价值S-K2，这点跟行权在数值上一致）。两腿一起
      结算离场。S≤K2时跟新版本行为一致（只平short call腿）。
      （注：早期版本这里曾用"行权覆盖"的方式结算，即final_pnl固定为
      (K2-K1)-net_cost+prev_premiums，会放弃long call当时剩余的时间
      价值——这是在模拟"放任不管、到期被自动行权/指派"的真实期权机制；
      现在改为更贴近"主动在市场上平仓两条腿"的假设，经济上更优，所以
      弃用了行权覆盖这个结算方式。）
  两种模式下，S≤K2（OTM）到期都是：权利金全额计入prev_premiums，转入
  dip_waiting，**没有"立即重开"选项**——必须等下一个UTurnTop信号才重开新
  short call（复用CoveredCallDipMixin的_check_dip_reopen）。

  dip_close_level机制：跟UTurnBSync_DebitCallSpreadEx基本一样，UTurnBottom
  信号可以在short call自然到期前提前买回它，让long call能吃到反弹涨幅不被
  封顶（复用CoveredCallDipMixin的_check_dip_close，不改一行）。唯一的区别
  是覆盖了_dip_buy_back_call，额外加一道门槛：dip_close_min_realized_ratio>0
  时，还要求short call权利金实现率（(call_prem-当前理论价)/call_prem，跟
  EX家族cc_roll_profit_ratio同一套公式）达到该比例才允许买回——UTurnBottom
  信号出现但权利金还没衰减够时不动作，等下一次信号再检查，不会提前放弃。

  long call持有满一定天数后主动展期（_check_long_call_roll/_roll_long_call）：
  跟UTurnBSync_CreditPutSpreadPMCC的_stock_leg_maintain/_roll_pmcc_leg类似的
  机制，但触发条件是"已经过去了多久"而不是"还剩多久"——自开仓/上次展期起已经
  过去了long_call_roll_after_days天时，旧leg按当前价格结算浮盈浮亏计入
  prev_premiums，在当前价位重新买入一份全新的long_call_days天期long call
  接续，跟short call当前状态无关（两条腿的生命周期独立）。

关键复用：CoveredCallDipMixin（_cc_dip_mixin.py）的dip_close/dip_reopen/
fill_check整套机制**完全不用改**，因为"到期后必须等UTurnTop才重开"正好
就是mixin里`dip_reopen_level>0`时的既有路径（`_check_dip_reopen`），不需要
"立即重开"（dip_reopen_level=0）那条分支。唯一自己写的、跟mixin行为不同的
地方是"short call自然到期"本身：mixin假设的sibling策略在OTM到期后会立即
`_make_call_fields`重开，而本策略要转入dip_waiting等信号；且无论ITM/OTM都
只结算short call这一条腿，不触碰long call。

命名约束：mixin的_check_dip_reopen/_check_pending_call_open/_dip_call_opt_days
直接硬编码引用self.roll_entry_short/self.roll_option_days/self.roll_expiry_weekday
这几个属性名，所以short call的delta/DTE/到期星期必须用这几个确切的名字（不能
自己另起entry_short/short_call_days），这里正好也符合语义——每一次short call
都是"滚仓"，不区分初次/后续。

  long call ↔ 正股 切换（_check_long_dip_close/_check_long_dip_reopen）：
  跟short call的dip机制是同一种模式，但信号源完全独立（各自一套
  long_dip_close_*/long_dip_reopen_*参数，sync/level/code_id都可以跟short
  call不同）——出现指定的UTurnBottom时，把long call按当前理论价平仓（浮盈
  浮亏计入prev_premiums），换成等量正股（记录持股基准价long_stock_s0）；等
  到指定的UTurnTop出现时，把正股平仓（浮盈浮亏同样计入prev_premiums），换
  回一份全新的long call（DTE/到期星期复用long_call_days/long_call_expiry_weekday，
  但delta用独立的long_dip_reopen_delta，不跟entry_long共用）。正股腿的点数
  盈亏是`S-long_stock_s0`，跟call权利金是同一套点数单位，不需要额外的股数
  概念（沿用EX基类_stock_leg_value的建模方式）。

  这个切换影响两处既有逻辑：
    - _check_long_call_roll（按天数展期）只在long_form=='call'时生效，持有
      正股期间跳过——没有"正股到期"这回事。
    - settle_itm_via_exercise=True（老版本）时，S>K2到期调用的_cc_finalize
      本身就按long_form分支处理：'call'形态按long call理论价市价平仓；
      'stock'形态按正股的线性盈亏（S-long_stock_s0，没有时间价值概念）
      结算，两种形态走的是同一套_cc_finalize逻辑，不需要额外分支。
      settle_itm_via_exercise=False（新版本）时，short call到期完全不
      触碰long leg，long_form是'call'还是'stock'对到期结算没有任何影响。
"""
from datetime import timedelta, datetime, time as dtime

from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy
from qib_trader.strategies._cc_dip_mixin import CoveredCallDipMixin
from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps, _load_bars
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing
from qib_trader.live.intent import emit_intent


class UTurnBSync_DebitCallSpreadPMCC(CoveredCallDipMixin, Strategy):
    """对角价差版Debit Call Spread：long call远期模拟正股，short call近期反复滚动。"""

    options_mode:    bool  = True
    initial_capital: float = 100_000.0

    sync_signals:       list  = []
    start_time:         str   = "2024-01-01 00:00:00"
    end_time:           str   = "2026-06-01 00:00:00"
    uturn_btm_level:    int   = 1
    multi_uturn_window: int   = 50
    min_dif_abs:        float = 0.0

    # ── 开仓：long call（远期，模拟正股）───────────────────────────────────────
    entry_long:               float = 0.85   # long call delta（比DebitCallSpreadEx的0.7更深实值）
    long_call_days:           int   = 200
    long_call_expiry_weekday: int   = -1
    long_call_roll_after_days: int = 100     # 自开仓/上次展期起已过去此天数时主动展期（0=不启用）

    # ── short call（近期，反复滚动；命名沿用现有家族的roll_*，供mixin直接复用）
    roll_entry_short:    float = 0.4
    roll_option_days:    int   = 14
    roll_expiry_weekday: int   = -1
    max_roll_count:      int   = 0   # 0=不限制

    # ── Dip 参数（UTurnBtm→提前买回short call；UTurnTop→重开，唯一重开路径）───
    dip_close_level:    int  = 0     # 0=不启用
    dip_close_sync:     bool = True
    dip_close_code_id:  str  = ""
    dip_close_window:   int  = 50
    dip_reopen_level:   int  = 1     # 必须>0：本策略不支持"立即重开"
    dip_reopen_sync:    bool = True
    dip_reopen_code_id: str  = ""
    dip_reopen_window:  int  = 50
    dip_roll_option_days: int = 0    # 0=使用roll_option_days
    dip_apply_to_spread:  bool = False  # mixin文档要求的属性；本策略永远phase='covered_call'，不会被实际读取
    dip_close_min_realized_ratio: float = 0.0  # short call权利金实现率需≥此比例才允许dip_close买回（0=不限制）

    # ── long call ↔ 正股 切换（独立于short call的dip信号）───────────────────────
    long_dip_close_level:    int  = 0     # 0=不启用；UTurnBottom→平long call换正股
    long_dip_close_sync:     bool = True
    long_dip_close_code_id:  str  = ""
    long_dip_close_window:   int  = 50
    long_dip_reopen_level:   int  = 1     # 必须>0：换回long call没有"立即重开"选项
    long_dip_reopen_sync:    bool = True
    long_dip_reopen_code_id: str  = ""
    long_dip_reopen_window:  int  = 50
    long_dip_reopen_delta:   float = 0.85  # 正股换回long call时用的delta，独立于entry_long

    # ── 共用参数 ───────────────────────────────────────────────────────────────
    min_vix:        float = 0.0
    vix_code_id:    str   = "NQ"
    min_drop_from_prev_close: float = 0.0  # 当前价须低于昨收此比例才开仓（0=不限制，如0.02=须跌破昨收2%）
    settle_itm_via_exercise: bool = False  # short call到期S>K2(ITM)时的处理方式：False(默认,新版本)=只平short腿,long call永不平仓；True(老版本)=行权long call覆盖,两腿一起平仓离场
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
        self._last_bar: BarData | None = None
        self._last_entry_dt = None
        self._sync_timestamps: set = set()
        self._dip_close_timestamps: set = set()
        self._dip_reopen_timestamps: set = set()
        self._long_dip_close_timestamps: set = set()
        self._long_dip_reopen_timestamps: set = set()

    def on_start(self) -> None:
        self.active = True
        if self.dip_reopen_level <= 0:
            self.logger.warning(
                "dip_reopen_level<=0 在本策略里不受支持（没有'立即重开'选项），"
                "已强制视为1（等待UTurnTop信号才重开）")
            self.dip_reopen_level = 1
        if self.long_dip_close_level > 0 and self.long_dip_reopen_level <= 0:
            self.logger.warning(
                "long_dip_close_level>0 但 long_dip_reopen_level<=0（换回long call"
                "没有'立即重开'选项），已强制视为1（等待UTurnTop信号才换回）")
            self.long_dip_reopen_level = 1
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
                self.dip_close_level, self.dip_close_window,
            )
        self._dip_reopen_timestamps = self._build_top_timestamps(
            self.dip_reopen_sync, self.dip_reopen_code_id,
            self.dip_reopen_level, self.dip_reopen_window,
        )
        if self.long_dip_close_level > 0:
            self._long_dip_close_timestamps = self._build_btm_timestamps(
                self.long_dip_close_sync, self.long_dip_close_code_id,
                self.long_dip_close_level, self.long_dip_close_window,
            )
            self._long_dip_reopen_timestamps = self._build_top_timestamps(
                self.long_dip_reopen_sync, self.long_dip_reopen_code_id,
                self.long_dip_reopen_level, self.long_dip_reopen_window,
            )
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
            self._cc_finalize(p, self._last_bar, 'force_close',
                              self._last_bar.close_price,
                              0.0 if p.get('dip_waiting', False) else
                              self._short_call_value(self._last_bar, p, self._last_bar.close_price))
        self._positions.clear()
        self._record_mtm_pnl(self._last_bar)

    # ── 通用工具（跟sibling策略文件一样，各自file内重复定义） ────────────────

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

    # ── short call 工具（_make_call_fields 是mixin要求的确切方法名） ──────────

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

    def _short_call_value(self, bar: BarData, p: dict, S: float) -> float:
        T = self._rem_t(bar.datetime, p['call_expiry'])
        return option_pricing.call_price(S, p['call_k'], T, self.risk_free_rate, p['call_iv'], self.codeId)

    def _dip_buy_back_call(self, p: dict, bar: BarData) -> bool:
        """覆盖mixin的默认实现，在"UTurnBtm信号出现"之外再加一道门槛：short call
        权利金实现率需≥dip_close_min_realized_ratio才允许买回（公式跟sibling
        策略EX家族的cc_roll_profit_ratio同一套：(call_prem-当前理论价)/call_prem，
        0=不限制，退回mixin原行为）。未达标时返回False——跟mixin自身"未通过无
        滑点成交检查"时的语义一致：position保持不动，等下一次dip_close信号
        （同一UTurnBtm周期内可能是同一根bar，也可能是之后某一根）再重新检查，
        不会因为这一次没达标就永久放弃，也不会提前消耗/改变任何仓位状态。"""
        if self.dip_close_min_realized_ratio > 0:
            if p['call_prem'] <= 0:
                return False
            curr_call = self._short_call_value(bar, p, bar.close_price)
            realized  = (p['call_prem'] - curr_call) / p['call_prem']
            if realized < self.dip_close_min_realized_ratio:
                return False
        return super()._dip_buy_back_call(p, bar)

    # ── long call 工具（远期模拟正股腿，跟PMCC的_stock_leg_*同样的机制） ──────

    def _make_long_call_fields(self, bar: BarData, S: float, delta: float = None) -> dict:
        """delta=None时使用entry_long（开仓/长期展期用）；正股换回long call时
        （_convert_stock_to_long）传long_dip_reopen_delta，用独立的delta。"""
        delta  = self.entry_long if delta is None else delta
        expiry = self._next_expiry(bar.datetime, self.long_call_days, self.long_call_expiry_weekday)
        T      = self._rem_t(bar.datetime, expiry)
        iv     = get_iv(self.codeId, 'call', delta)
        k      = option_pricing.call_strike_by_delta(
            S, T, delta, iv, self.risk_free_rate, self.codeId, self.strike_round)
        prem   = option_pricing.call_price(S, k, T, self.risk_free_rate, iv, self.codeId)
        return {'long_call_k': k, 'long_call_prem': prem, 'long_call_iv': iv,
                'long_call_expiry': expiry, 'long_call_entry_dt': bar.datetime}

    def _long_leg_value(self, bar: BarData, p: dict, S: float) -> float:
        """long leg（call形态或正股形态）在标的价格为S时对总盈亏的贡献。
        call形态 = 当前理论价 - 买入权利金；正股形态 = S - 持股基准价。"""
        if p['long_form'] == 'stock':
            return S - p['long_stock_s0']
        T    = self._rem_t(bar.datetime, p['long_call_expiry'])
        curr = option_pricing.call_price(S, p['long_call_k'], T, self.risk_free_rate, p['long_call_iv'], self.codeId)
        return curr - p['long_call_prem']

    def _check_long_call_roll(self, bar: BarData) -> None:
        """自开仓/上次展期起已过去long_call_roll_after_days天时主动展期，跟
        short call当前状态无关（两条腿的生命周期独立）。正股形态没有"到期"
        这回事，跳过。"""
        if self.long_call_roll_after_days <= 0:
            return
        for p in self._positions:
            if p['long_form'] != 'call':
                continue
            days_elapsed = (bar.datetime - p['long_call_entry_dt']).days
            if days_elapsed < self.long_call_roll_after_days:
                continue
            self._roll_long_call(p, bar)

    def _roll_long_call(self, p: dict, bar: BarData) -> None:
        """平掉即将到期的旧long call（浮盈浮亏计入prev_premiums），在当前
        价位重新买入一份全新的long_call_days天期long call接续。"""
        S        = bar.close_price
        curr_old = self._long_leg_value(bar, p, S) + p['long_call_prem']  # 旧leg当前理论价
        realized = curr_old - p['long_call_prem']
        p['prev_premiums'] += realized
        old_k, old_expiry = p['long_call_k'], p['long_call_expiry']
        emit_intent(self, 'long_call_roll_close', p, bar, right='call', leg_name='long_call',
                    old_k=old_k, old_prem=p['long_call_prem'], close_price=curr_old)
        self.logger.info(
            f"[{bar.datetime}] Long call到期前主动展期: 平旧leg k={old_k} exp={old_expiry.date()} "
            f"curr={curr_old:.4f} realized={realized:.4f}"
        )
        p.update(self._make_long_call_fields(bar, S))
        emit_intent(self, 'long_call_open', p, bar, right='call', leg_name='long_call',
                    new_k=p['long_call_k'], new_prem=p['long_call_prem'], new_expiry=p['long_call_expiry'],
                    target_delta=self.entry_long, target_days=self.long_call_days,
                    expiry_weekday=self.long_call_expiry_weekday)

    # ── long call ↔ 正股 切换（独立于short call的dip信号）─────────────────────

    def _check_long_dip_close(self, bar: BarData) -> None:
        """指定的UTurnBottom：把long call平仓换成正股。"""
        if self.long_dip_close_level <= 0:
            return
        if not self._long_dip_close_timestamps or bar.datetime not in self._long_dip_close_timestamps:
            return
        for p in self._positions:
            if p['long_form'] != 'call' or p.get('long_dip_waiting', False):
                continue
            self._convert_long_to_stock(p, bar)

    def _convert_long_to_stock(self, p: dict, bar: BarData) -> None:
        S        = bar.close_price
        curr     = self._long_leg_value(bar, p, S) + p['long_call_prem']
        realized = curr - p['long_call_prem']
        p['prev_premiums'] += realized
        old_k, old_expiry = p['long_call_k'], p['long_call_expiry']
        emit_intent(self, 'long_call_to_stock', p, bar, right='call', leg_name='long_call',
                    old_k=old_k, old_prem=p['long_call_prem'], close_price=curr)
        self.logger.info(
            f"[{bar.datetime}] Long call平仓换正股: k={old_k} exp={old_expiry.date()} "
            f"curr={curr:.4f} realized={realized:.4f} 正股基准价={S:.2f}"
        )
        p['long_call_k'] = p['long_call_prem'] = p['long_call_iv'] = 0.0
        p['long_form']       = 'stock'
        p['long_stock_s0']   = S
        p['long_dip_waiting'] = True

    def _check_long_dip_reopen(self, bar: BarData) -> None:
        """指定的UTurnTop：把正股平仓换回long call（复用entry_long/long_call_days/
        long_call_expiry_weekday，跟开仓/长期展期同一组参数）。"""
        if self.long_dip_reopen_level <= 0:
            return
        if not self._long_dip_reopen_timestamps or bar.datetime not in self._long_dip_reopen_timestamps:
            return
        for p in self._positions:
            if p['long_form'] != 'stock' or not p.get('long_dip_waiting', False):
                continue
            self._convert_stock_to_long(p, bar)

    def _convert_stock_to_long(self, p: dict, bar: BarData) -> None:
        S        = bar.close_price
        s0       = p['long_stock_s0']
        realized = S - s0
        p['prev_premiums'] += realized
        emit_intent(self, 'stock_to_long_call', p, bar, right='stock', leg_name='long_call',
                    old_prem=s0, close_price=S)
        self.logger.info(
            f"[{bar.datetime}] 正股平仓换回long call: 基准价={s0:.2f} curr={S:.2f} realized={realized:.4f}"
        )
        p['long_stock_s0']    = None
        p['long_dip_waiting'] = False
        p['long_form']        = 'call'
        p.update(self._make_long_call_fields(bar, S, delta=self.long_dip_reopen_delta))
        emit_intent(self, 'long_call_open', p, bar, right='call', leg_name='long_call',
                    new_k=p['long_call_k'], new_prem=p['long_call_prem'], new_expiry=p['long_call_expiry'],
                    target_delta=self.long_dip_reopen_delta, target_days=self.long_call_days,
                    expiry_weekday=self.long_call_expiry_weekday)

    # ── short call 自然到期结算 ──────────────────────────────────────────────

    def _check_short_call_expiry(self, bar: BarData) -> None:
        remaining = []
        for p in list(self._positions):
            if not p.get('dip_waiting', False) and bar.datetime >= p['call_expiry']:
                if self._settle_short_call_expiry(p, bar):
                    remaining.append(p)
            else:
                remaining.append(p)
        self._positions = remaining

    def _settle_short_call_expiry(self, p: dict, bar: BarData) -> bool:
        """True=position继续（转dip_waiting，等UTurnTop重开）；False=仅在
        settle_itm_via_exercise=True且S>K2时发生，两腿一起市价平仓离场，
        lifecycle结束。

        settle_itm_via_exercise控制S>K2（ITM）到期时的处理方式：
          False（默认，新版本）：无论ITM/OTM都只平short call这一条腿，long
            call永不平仓、永不行权，继续持有/按长期展期机制滚动。
          True（老版本）：S>K2时两腿一起按各自理论价市价平仓离场（复用
            _cc_finalize——long call按bar.close_price算出的理论价卖出，
            保留其剩余时间价值，不当成行权强制归零）；S≤K2时跟新版本
            行为一致。
        """
        S, K2 = bar.close_price, p['call_k']
        if self.settle_itm_via_exercise and S > K2:
            call_close = self._short_call_value(bar, p, S)   # T=0时自然收敛为内在价值S-K2
            self._cc_finalize(p, bar, 'expiry_exercise', S, call_close)
            return False
        # 只平short call这一条腿：统一按"权利金-到期时内在价值"结算
        # （_short_call_value在T=0时自然收敛到max(S-K2,0)，OTM时恰好为0，
        #  等价于"权利金全额计入"）。long call完全不受影响。
        old_k, old_prem = p['call_k'], p['call_prem']
        settle_value = self._short_call_value(bar, p, S)
        realized = old_prem - settle_value
        p['prev_premiums'] += realized
        p['call_k'] = p['call_prem'] = p['call_iv'] = 0.0
        p['dip_waiting'] = True
        event = 'otm_expiry' if settle_value <= 0 else 'itm_expiry'
        self._append_roll_history(p, bar, event,
                                  old_call_k=old_k, old_call_prem=old_prem, buyback_price=settle_value)
        self.logger.info(
            f"[{bar.datetime}] Short call到期结算({event}): k={old_k} S={S:.2f} "
            f"settle_value={settle_value:.4f} realized={realized:.4f}（long call不受影响），等待UTurnTop重开"
        )
        return True

    def _cc_finalize(self, p: dict, bar: BarData, close_reason: str,
                     stock_exit: float, call_close: float) -> None:
        """通用平仓：long leg（call形态按当前理论价卖出，不是行权覆盖；正股
        形态直接按市价卖出）加上累计权利金和（若有正在途的short call）买回
        成本。调用方：mixin的max_roll_exceeded路径、on_stop的force_close，
        以及settle_itm_via_exercise=True时_settle_short_call_expiry里S>K2
        的两腿市价平仓——这三种情况都是"两条腿按各自理论价结算"，不是行权。
        stock_exit这个参数名是为了满足mixin的统一调用约定（传bar.close_price），
        只是用来给long leg定价的S，跟long_form=='stock'时的正股是两回事。"""
        long_leg  = self._long_leg_value(bar, p, stock_exit)
        final_pnl = long_leg + p['prev_premiums'] + (p['call_prem'] - call_close)
        if p['long_form'] == 'stock':
            emit_intent(self, 'stock_close', p, bar, right='stock', leg_name='long_call',
                        old_prem=p['long_stock_s0'], close_price=stock_exit, close_reason=close_reason)
        else:
            emit_intent(self, 'long_call_close', p, bar, right='call', leg_name='long_call',
                        old_k=p['long_call_k'], old_prem=p['long_call_prem'],
                        close_price=long_leg + p['long_call_prem'], close_reason=close_reason)
        self._finalize(p, bar, close_reason, final_pnl)

    def _finalize(self, p: dict, bar: BarData, close_reason: str, final_pnl: float) -> None:
        """结束lifecycle，发出合成SHORT订单（与entry LONG @ net_cost配对），记录导出。"""
        exit_price = p['net_cost'] + final_pnl
        self._send_order(Direction.SHORT, exit_price, 1, "MKT")
        self._closed_positions.append({
            'position_id':      p['position_id'], 'phase': 'covered_call',
            'close_reason':     close_reason,
            'entry_dt':         p['entry_dt'],
            'settlement_dt':    bar.datetime,
            'entry_underlying': p['entry_underlying'],
            'long_form':        p['long_form'],
            'long_stock_s0':    p['long_stock_s0'],
            'long_call_k':      p['long_call_k'],
            'long_call_prem':   p['long_call_prem'],
            'long_call_expiry': p['long_call_expiry'],
            'call_k':           p['call_k'],
            'call_prem':        p['call_prem'],
            'net_cost':         p['net_cost'],
            'roll_count':       p['roll_count'],
            'prev_premiums':    p['prev_premiums'],
            'final_pnl':        final_pnl,
        })
        self.logger.info(
            f"[{bar.datetime}] 平仓 {close_reason} rolls={p['roll_count']} "
            f"long_call_k={p['long_call_k']:.2f} net_cost={p['net_cost']:.4f} pnl={final_pnl:.4f}"
        )

    # ── 盯市 ──────────────────────────────────────────────────────────────────

    def _calc_mtm_pnl(self, bar: BarData) -> float:
        if not self._positions:
            return 0.0
        S, total = bar.close_price, 0.0
        for p in self._positions:
            long_leg = self._long_leg_value(bar, p, S)
            if p.get('dip_waiting', False):
                total += long_leg - p['net_cost'] + p['prev_premiums']
            else:
                curr_call = self._short_call_value(bar, p, S)
                total += long_leg - p['net_cost'] + p['prev_premiums'] + (p['call_prem'] - curr_call)
        return total

    # ── 开仓 ──────────────────────────────────────────────────────────────────

    def _entry_signal(self, bar: BarData) -> bool:
        if bar.datetime not in self._sync_timestamps:
            return False
        if self.min_drop_from_prev_close > 0:
            price_below_prev_close = (bar.prev_day_close <= 0
                                      or bar.close_price < bar.prev_day_close * (1 - self.min_drop_from_prev_close))
            if not price_below_prev_close:
                return False
        if self.min_dif_abs > 0 and abs(bar.dif) < self.min_dif_abs:
            return False
        if self.min_vix > 0:
            vix = (self._vix_map.get(bar.datetime, 0.0) if self.vix_code_id
                   else (bar.index_close if bar.index_valid else 0.0))
            if vix < self.min_vix:
                return False
        if (self._last_entry_dt is not None and self.cooldown_hours > 0
                and bar.datetime < self._last_entry_dt + timedelta(hours=self.cooldown_hours)):
            return False
        return not (self.max_positions > 0 and len(self._positions) >= self.max_positions)

    def _execute_entry(self, bar: BarData) -> None:
        S = bar.close_price
        long_flds  = self._make_long_call_fields(bar, S)
        short_flds = self._make_call_fields(bar, S, self.roll_entry_short,
                                            self.roll_option_days, self.roll_expiry_weekday)
        net_cost = long_flds['long_call_prem'] - short_flds['call_prem']
        self._last_entry_dt = bar.datetime
        self._position_seq += 1
        p = {
            'position_id': self._position_seq, 'phase': 'covered_call',
            'entry_dt': bar.datetime, 'entry_underlying': S,
            'net_cost': net_cost, 'prev_premiums': 0.0, 'roll_count': 0,
            'dip_waiting': False,
            'long_form': 'call', 'long_stock_s0': None, 'long_dip_waiting': False,
        }
        p.update(long_flds)
        p.update(short_flds)
        self._positions.append(p)
        self._send_order(Direction.LONG, net_cost, 1, "MKT")
        self.logger.info(
            f"[{bar.datetime}] 开仓 DebitCallSpreadPMCC long_k={p['long_call_k']} "
            f"(exp={p['long_call_expiry'].date()}) short_k={p['call_k']} "
            f"(exp={p['call_expiry'].date()}) net_cost={net_cost:.4f}"
        )
        emit_intent(self, 'long_call_open', p, bar, right='call', leg_name='long_call',
                    new_k=p['long_call_k'], new_prem=p['long_call_prem'], new_expiry=p['long_call_expiry'],
                    target_delta=self.entry_long, target_days=self.long_call_days,
                    expiry_weekday=self.long_call_expiry_weekday)
        self._append_roll_history(p, bar, 'cc_open',
                                  new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                  new_expiry=p['call_expiry'])

    def _check_entry(self, bar: BarData) -> None:
        if self._entry_signal(bar):
            self._execute_entry(bar)

    # ── 主逻辑 ────────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_short_call_expiry(bar)   # short call自然到期（settle_itm_via_exercise控制ITM处理方式）
        self._check_dip_close(bar)           # UTurnBtm：提前买回short call
        self._check_dip_reopen(bar)          # UTurnTop：唯一的重开路径
        self._check_pending_call_open(bar)   # 逐bar重试因未通过成交检查而挂起的开仓（本策略暂未触发，保留兼容）
        self._check_long_dip_close(bar)      # 独立信号 UTurnBtm：long call平仓换正股
        self._check_long_dip_reopen(bar)     # 独立信号 UTurnTop：正股平仓换回long call
        self._check_long_call_roll(bar)      # long call到期前主动展期（仅call形态生效）
        self._check_entry(bar)
        self._record_mtm_pnl(bar)
        self._last_bar = bar
