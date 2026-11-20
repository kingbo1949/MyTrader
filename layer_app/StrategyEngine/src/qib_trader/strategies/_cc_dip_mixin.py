"""
CoveredCallDipMixin：CC 滚动态（及可选价差态）Dip 机制公共逻辑。

三个策略共用：
  UTurnBSync_CoveredCallEx      → CoveredCallDipMixin（无价差阶段，_dip_*_spread_leg 为 no-op）
  UTurnBSync_CreditPutSpreadEx  → CoveredCallDipMixin + 覆盖 _dip_{close,reopen}_spread_leg
  UTurnBSync_DebitCallSpreadEx  → CoveredCallDipMixin + 覆盖 _dip_{close,reopen}_spread_leg

宿主类须通过类属性 / 实例属性 / 方法提供：
  属性：sync_signals, interval, codeId, start_time, end_time, risk_free_rate
        dip_close_level/sync/code_id/window, dip_reopen_level/sync/code_id/window
        dip_apply_to_spread, max_roll_count, roll_entry_short, roll_option_days,
        roll_expiry_weekday
  方法：_rem_t(), _make_call_fields(), _cc_finalize(), logger
  状态：_positions (list[dict]), _dip_close_timestamps (set), _dip_reopen_timestamps (set)
        dip_roll_option_days（int，0=使用 roll_option_days）
        _roll_log（list[dict]，可选：若宿主提供则每次滚仓/展期会追加一条扁平记录，
        用于导出独立的滚仓明细文件；未提供时仅记录到 p['roll_history']，不导出）
        fill_check_bars（int，可选，默认0：>0 时启用无滑点成交可行性检查，需宿主提供
        _peek_next_bars()，见 strategies/base.py）
        fill_check_spread（float，可选，默认1.0：假设的期权 bid/ask 宽度）

本 mixin 会在每次通过 dip 机制（_check_dip_reopen/_dip_roll_immediate_call/挂起重试里
event=='dip_reopen_immediate' 的分支）开出新 call 时，把 p['from_dip_reopen'] 设为 True；
宿主类正常开仓路径（_transition_to_cc/到期滚仓/profit_roll 重开）应将其设为 False。这个
标记只是留给宿主类自己决定怎么用（比如 UTurnBSync_CreditPutSpreadEx 用它 + 一个开关参数
决定 dip_reopen 来的 call 要不要参与 cc_early_profit 提前平仓判断）——mixin 本身不读它。
"""

from qib_trader.strategies.uturn_bsync_credit_put_spread import _build_sync_timestamps
from qib_trader.strategies.uturn_bsync_debit_put_spread import _build_sync_timestamps_top
from qib_trader.utils import option_pricing
from qib_trader.live.intent import emit_intent


class CoveredCallDipMixin:
    """CC 滚动态 Dip 公共逻辑：时间戳构建 / 买回 call / 立即展期 / dip close+reopen 检查。"""

    # ── 时间戳构建 ────────────────────────────────────────────────────────────

    def _build_btm_timestamps(self, sync: bool, code_id: str,
                               level: int, window: int) -> set:
        """构建 UTurnBottom 时间戳集合（同步模式或单品种模式）。"""
        signals = (self.sync_signals if sync
                   else [{'code_id': code_id or self.codeId, 'min_dif_abs': 0.0}])
        if not signals:
            return set()
        return _build_sync_timestamps(signals, self.interval.value,
                                      self.start_time, self.end_time,
                                      level=level, window=window)

    def _build_top_timestamps(self, sync: bool, code_id: str,
                               level: int, window: int) -> set:
        """构建 UTurnTop 时间戳集合（同步模式或单品种模式）。"""
        signals = (self.sync_signals if sync
                   else [{'code_id': code_id or self.codeId, 'min_dif_abs': 0.0}])
        if not signals:
            return set()
        return _build_sync_timestamps_top(signals, self.interval.value,
                                          self.start_time, self.end_time,
                                          level=level, window=window)

    # ── 辅助 ─────────────────────────────────────────────────────────────────

    def _dip_call_opt_days(self) -> int:
        """dip 重建 short call 时使用的到期天数。
        dip_roll_option_days > 0 时使用该值，否则沿用常规滚仓的 roll_option_days。"""
        days = getattr(self, 'dip_roll_option_days', 0)
        return days if days > 0 else self.roll_option_days

    def _append_roll_history(self, p: dict, bar, event: str,
                             old_call_k: float = None, old_call_prem: float = None,
                             buyback_price: float = None,
                             new_call_k: float = None, new_call_prem: float = None,
                             new_expiry=None) -> None:
        """记录一次滚仓/展期事件。写入 p['roll_history']（内部用于 seq 计数），
        若宿主策略提供了 self._roll_log（列表），同时追加一条扁平记录，
        用于导出为独立的滚仓明细文件（按 position_id 关联主记录）。"""
        history  = p.setdefault('roll_history', [])
        realized = (round(old_call_prem - buyback_price, 4)
                   if old_call_prem is not None and buyback_price is not None else None)
        record = {
            'seq': len(history) + 1, 'dt': bar.datetime, 'event': event,
            'underlying': bar.close_price,
            'old_call_k': old_call_k,
            'old_call_prem': round(old_call_prem, 4) if old_call_prem is not None else None,
            'buyback_price': round(buyback_price, 4) if buyback_price is not None else None,
            'realized_premium': realized,
            'new_call_k': new_call_k,
            'new_call_prem': round(new_call_prem, 4) if new_call_prem is not None else None,
            'new_expiry': new_expiry,
        }
        history.append(record)
        roll_log = getattr(self, '_roll_log', None)
        if roll_log is not None:
            roll_log.append({'position_id': p.get('position_id'), **record})
        # 实盘意图捕获（回测/无 sink 时 emit_intent 是 no-op）：target_days 从已经算好的
        # new_expiry 反推，而不是假设某个固定的 xxx_option_days 属性——这样无论是常规滚仓
        # 还是 dip 相关滚仓（用的是 _dip_call_opt_days()）都能拿到准确的目标 DTE。
        target_days = (new_expiry - bar.datetime).days if new_expiry is not None else None
        emit_intent(self, event, p, bar, right='call',
                    old_k=old_call_k, old_prem=old_call_prem, close_price=buyback_price,
                    new_k=new_call_k, new_prem=new_call_prem, new_expiry=new_expiry,
                    target_delta=self.roll_entry_short if new_call_k is not None else None,
                    target_days=target_days,
                    expiry_weekday=getattr(self, 'roll_expiry_weekday', -1))

    # ── 无滑点成交可行性检查 ──────────────────────────────────────────────────

    def _fill_check(self, bar, direction: str, K: float, T: float, iv: float,
                    S0: float = None) -> bool:
        """无滑点成交可行性检查：fill_check_bars<=0 时恒为 True（保持瞬时成交行为）。
        direction='BUY'（平仓旧call，检查后续bar低点下穿）/'SELL'（开新call，检查后续bar高点上穿）。
        只判断能否成交，不改变成交价格——成交价始终是调用方按信号bar算出的理论价。"""
        n = getattr(self, 'fill_check_bars', 0)
        if n <= 0:
            return True
        if not self._own_bars and not getattr(self, '_fill_check_warned', False):
            self._fill_check_warned = True
            self.logger.warning("fill_check_bars>0 但 _own_bars 未注入，无滑点检查将恒为不成交")
        if S0 is None:
            S0 = bar.close_price
        spread = getattr(self, 'fill_check_spread', 1.0)
        delta  = max(abs(option_pricing.call_delta(S0, K, T, self.risk_free_rate, iv, self.codeId)), 1e-6)
        dS     = spread / delta
        peek   = self._peek_next_bars(n)
        if direction == 'BUY':
            return any(b.low_price <= S0 - dS for b in peek)
        return any(b.high_price >= S0 + dS for b in peek)

    def _open_call_checked(self, bar, S: float, delta: float, opt_days: int, exp_weekday: int) -> dict | None:
        """SELL-open 版 _make_call_fields：K/expiry/iv/prem 全部按信号价 S 计算（与现有行为一致），
        未通过可行性检查时返回 None，调用方不得修改 p。"""
        flds = self._make_call_fields(bar, S, delta, opt_days, exp_weekday)
        T = self._rem_t(bar.datetime, flds['call_expiry'])
        if not self._fill_check(bar, 'SELL', flds['call_k'], T, flds['call_iv'], S0=S):
            return None
        return flds

    def _close_call_checked(self, bar, p: dict) -> tuple:
        """BUY-close 版：返回 (是否成交, 成交价)。成交价与现有 curr_call 计算方式完全一致。"""
        T = self._rem_t(bar.datetime, p['call_expiry'])
        curr_call = option_pricing.call_price(bar.close_price, p['call_k'], T,
                                              self.risk_free_rate, p['call_iv'], self.codeId)
        if not self._fill_check(bar, 'BUY', p['call_k'], T, p['call_iv'], S0=bar.close_price):
            return False, 0.0
        return True, curr_call

    def _check_pending_call_open(self, bar) -> None:
        """逐 bar 重试因未通过成交检查而挂起的开仓（SELL-open）。
        处理 cc_open/expiry_otm_roll/dip_reopen_immediate 这三类没有离散信号可等的场景。"""
        if getattr(self, 'fill_check_bars', 0) <= 0:
            return
        remaining = []
        for p in self._positions:
            if not p.get('call_open_pending', False):
                remaining.append(p)
                continue
            flds = self._open_call_checked(bar, bar.close_price, self.roll_entry_short,
                                            p['call_open_pending_days'], self.roll_expiry_weekday)
            if flds is None:
                remaining.append(p)
                continue
            new_roll = p['roll_count'] + 1
            if self.max_roll_count > 0 and new_roll > self.max_roll_count:
                self._cc_finalize(p, bar, 'max_roll_exceeded', bar.close_price, 0.0)
                continue
            p['roll_count'] = new_roll
            p.update(flds)
            p['dip_waiting'] = False
            event = p.pop('call_open_pending_event')
            p['from_dip_reopen'] = (event == 'dip_reopen_immediate')
            p.pop('call_open_pending_days', None)
            p['call_open_pending'] = False
            self._append_roll_history(p, bar, event, new_call_k=p['call_k'],
                                      new_call_prem=p['call_prem'], new_expiry=p['call_expiry'])
            self.logger.info(f"[{bar.datetime}] 挂起开仓重试成交#{p['roll_count']} event={event}")
            remaining.append(p)
        self._positions = remaining

    # ── CC 阶段 Dip 动作 ──────────────────────────────────────────────────────

    def _dip_buy_back_call(self, p: dict, bar) -> bool:
        """尝试买回 short call。未通过无滑点成交检查时返回 False，不改变 p 任何字段。
        成交时 realized premium 并入 prev_premiums，清空 call 字段，返回 True。"""
        filled, curr_call = self._close_call_checked(bar, p)
        if not filled:
            self.logger.info(f"[{bar.datetime}] 下跌平call 未通过成交检查，维持原call，等待下次UTurnBottom信号")
            return False
        realized = p['call_prem'] - curr_call
        self._append_roll_history(p, bar, 'dip_buyback',
                                  old_call_k=p['call_k'], old_call_prem=p['call_prem'],
                                  buyback_price=curr_call)
        p['prev_premiums'] += realized
        p['call_prem'] = 0.0
        p['call_k']    = 0.0
        p['call_iv']   = 0.0
        self.logger.info(
            f"[{bar.datetime}] 下跌平call: curr_call={curr_call:.4f} realized={realized:.4f}"
        )
        return True

    def _dip_roll_immediate_call(self, p: dict, bar) -> bool:
        """立即在当前价位重建 short call（dip_reopen_level=0）。
        未通过成交检查时进入 call_open_pending 挂起态，逐bar重试（此分支没有离散信号可等）。
        True=position 继续，False=lifecycle 结束（max_roll_exceeded）。"""
        new_roll = p['roll_count'] + 1
        if self.max_roll_count > 0 and new_roll > self.max_roll_count:
            self._cc_finalize(p, bar, 'max_roll_exceeded', bar.close_price, 0.0)
            return False
        flds = self._open_call_checked(bar, bar.close_price, self.roll_entry_short,
                                        self._dip_call_opt_days(), self.roll_expiry_weekday)
        if flds is None:
            p['dip_waiting'] = True
            p['call_open_pending'] = True
            p['call_open_pending_days'] = self._dip_call_opt_days()
            p['call_open_pending_event'] = 'dip_reopen_immediate'
            self.logger.info(f"[{bar.datetime}] 下跌立即展期call 未通过成交检查，转为逐bar重试")
            return True
        p['roll_count'] = new_roll
        p.update(flds)
        p['from_dip_reopen'] = True
        self._append_roll_history(p, bar, 'dip_reopen_immediate',
                                  new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                  new_expiry=p['call_expiry'])
        self.logger.info(
            f"[{bar.datetime}] 下跌立即展期call#{new_roll} "
            f"call_k={p['call_k']} prem={p['call_prem']:.4f} exp={p['call_expiry'].date()}"
        )
        return True

    # ── 价差阶段 Dip 钩子（无价差阶段的策略保持默认 no-op）─────────────────

    def _dip_close_spread_leg(self, p: dict, bar) -> None:
        """价差阶段 UTurnBtm：关闭一条腿（子类按需覆盖）。"""

    def _dip_reopen_spread_leg(self, p: dict, bar) -> None:
        """价差阶段 UTurnTop：重建一条腿（子类按需覆盖）。"""

    # ── 统一 Dip 检查 ─────────────────────────────────────────────────────────

    def _check_dip_close(self, bar) -> None:
        """UTurnBtm 触发时：CC 阶段买回 call；价差阶段（若启用）调用 _dip_close_spread_leg。"""
        if self.dip_close_level <= 0:
            return
        if not self._dip_close_timestamps or bar.datetime not in self._dip_close_timestamps:
            return
        remaining = []
        for p in self._positions:
            phase = p.get('phase', 'covered_call')
            if phase == 'covered_call' and not p.get('dip_waiting', False):
                if not self._dip_buy_back_call(p, bar):
                    remaining.append(p)
                elif self.dip_reopen_level > 0:
                    p['dip_waiting'] = True
                    remaining.append(p)
                elif self._dip_roll_immediate_call(p, bar):
                    remaining.append(p)
            elif (phase == 'spread'
                  and self.dip_apply_to_spread
                  and not p.get('spread_dip_waiting', False)):
                self._dip_close_spread_leg(p, bar)
                if self.dip_reopen_level == 0:
                    self._dip_reopen_spread_leg(p, bar)   # 立即重建
                remaining.append(p)
            else:
                remaining.append(p)
        self._positions = remaining

    def _check_dip_reopen(self, bar) -> None:
        """UTurnTop 触发时：CC 阶段重建 short call；价差阶段重建一条腿。"""
        if self.dip_reopen_level <= 0:
            return
        if not self._dip_reopen_timestamps or bar.datetime not in self._dip_reopen_timestamps:
            return
        remaining = []
        for p in self._positions:
            phase = p.get('phase', 'covered_call')
            if (phase == 'covered_call' and p.get('dip_waiting', False)
                    and not p.get('call_open_pending', False)):
                new_roll = p['roll_count'] + 1
                if self.max_roll_count > 0 and new_roll > self.max_roll_count:
                    self._cc_finalize(p, bar, 'max_roll_exceeded', bar.close_price, 0.0)
                    continue
                flds = self._open_call_checked(bar, bar.close_price, self.roll_entry_short,
                                                self._dip_call_opt_days(), self.roll_expiry_weekday)
                if flds is None:
                    self.logger.info(f"[{bar.datetime}] 反弹开call 未通过成交检查，维持等待，等下次UTurnTop信号")
                else:
                    p['roll_count'] = new_roll
                    p.update(flds)
                    p['dip_waiting'] = False
                    p['from_dip_reopen'] = True
                    self._append_roll_history(p, bar, 'dip_reopen',
                                              new_call_k=p['call_k'], new_call_prem=p['call_prem'],
                                              new_expiry=p['call_expiry'])
                    self.logger.info(
                        f"[{bar.datetime}] 反弹开call#{new_roll} "
                        f"call_k={p['call_k']} prem={p['call_prem']:.4f} exp={p['call_expiry'].date()}"
                    )
            elif phase == 'spread' and p.get('spread_dip_waiting', False):
                self._dip_reopen_spread_leg(p, bar)
            remaining.append(p)
        self._positions = remaining
