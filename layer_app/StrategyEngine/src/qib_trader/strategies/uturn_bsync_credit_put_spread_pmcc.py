"""
UTurnBSync_CreditPutSpreadPMCC —— 跟 UTurnBSync_CreditPutSpreadEx 完全一致，
唯一区别是 Phase1 接货时不买正股，改买一份深度实值、约 pmcc_days 天到期的
long call 模拟正股（Poor Man's Covered Call）：

  - 用远少于正股的资金（权利金 ≈ 内在价值+少量时间价值）换取接近1:1的方向暴露；
  - 下跌时最大亏损被限制在这份权利金以内（正股理论上能跌到0全损，long call
    最多亏掉买入时付的权利金）；
  - 代价：delta<1（不是完全跟涨跌）、这份call本身也有时间价值损耗、拿不到分红。

CC phase 结束（不管什么原因：到期行权/max_roll_exceeded/cc_early_profit/
force_close）时，平掉这份 long call，而不是"卖正股"。

除了"正股腿"这几个钩子（_stock_leg_open/_stock_leg_value/_stock_leg_close/
_stock_leg_maintain/_extra_record_fields，定义见
uturn_bsync_credit_put_spread_ex.py），其余全部逻辑（信号判断、Phase1价差
开平仓、Phase2滚仓/dip机制的触发条件）完全复用 UTurnBSync_CreditPutSpreadEx，
一行未改。

LEAP 到期前主动展期（_stock_leg_maintain/_roll_pmcc_leg）：
如果放任这份 long call 一直不管，CC phase 持仓时间一旦超过 pmcc_days（默认
200天，需要 max_roll_count=0 不限制滚仓次数才可能出现），_rem_t 会让它的剩余
期限钳制在0，_stock_leg_value 退化成纯内在价值 max(S-pmcc_k, 0)——这不是简单的
"提前结算"，而是会凭空产生一个真实市场里不存在的免费保护：只要标的价格跌破
pmcc_k，这份"到期"的头寸就会一直冻结在0，不再继续亏损，等于免费重新获得了
下跌保护，而真实到期后要么被行权变成真实正股（继续正常亏损）、要么现金结算
（一次性了结，之后跟这份期权毫无关系），都不会有这种效果。所以在剩余天数
降到 pmcc_roll_before_days 以内时，主动把这份 long call 平掉（浮盈浮亏计入
prev_premiums，语义上跟 short call 滚仓时的做法一致）、并在当前价位重新买入
一份全新的 pmcc_days 天期、pmcc_target_delta 的 long call 接续。
"""
from qib_trader.core.models import BarData
from qib_trader.strategies.uturn_bsync_credit_put_spread_ex import UTurnBSync_CreditPutSpreadEx
from qib_trader.utils.config_loader import get_iv
from qib_trader.utils import option_pricing
from qib_trader.live.intent import emit_intent


class UTurnBSync_CreditPutSpreadPMCC(UTurnBSync_CreditPutSpreadEx):
    """Credit Put Spread → 行权后用深度实值远期call（PMCC）代替正股滚仓。"""

    pmcc_days:            int   = 200    # 模拟正股的 long call 到期天数
    pmcc_target_delta:    float = 0.90   # 目标 |delta|，越接近1越像正股（但权利金越贵）
    pmcc_expiry_weekday:  int   = -1
    pmcc_roll_before_days: int  = 30     # 剩余天数 ≤ 此值时主动展期这份long call（0=不启用，退回旧行为）

    def _stock_leg_open(self, p: dict, bar: BarData) -> None:
        """买入模拟正股的 long call。接货时（_transition_to_cc 里 p['S0'] 设置
        之后）调用一次；到期前主动展期（_roll_pmcc_leg）时也复用这个方法开新leg。"""
        S      = bar.close_price
        expiry = self._next_expiry(bar.datetime, self.pmcc_days, self.pmcc_expiry_weekday)
        T      = self._rem_t(bar.datetime, expiry)
        iv     = get_iv(self.codeId, 'call', self.pmcc_target_delta)
        k      = option_pricing.call_strike_by_delta(
            S, T, self.pmcc_target_delta, iv, self.risk_free_rate, self.codeId, self.strike_round)
        prem   = option_pricing.call_price(S, k, T, self.risk_free_rate, iv, self.codeId)
        p['pmcc_k'], p['pmcc_prem'], p['pmcc_iv'] = k, prem, iv
        p['pmcc_expiry'], p['pmcc_entry_dt'] = expiry, bar.datetime
        self.logger.info(
            f"[{bar.datetime}] PMCC开仓(模拟正股): S={S:.2f} k={k} "
            f"prem={prem:.4f} delta目标={self.pmcc_target_delta} exp={expiry.date()}"
        )
        emit_intent(self, 'pmcc_open', p, bar, right='call', leg_name='PMCC_stock',
                    new_k=k, new_prem=prem, new_expiry=expiry,
                    target_delta=self.pmcc_target_delta, target_days=self.pmcc_days,
                    expiry_weekday=self.pmcc_expiry_weekday)

    def _stock_leg_value(self, bar: BarData, p: dict, S: float) -> float:
        """PMCC long call 在标的价格为 S 时对总盈亏的贡献 = 当前理论价 - 买入权利金。"""
        T    = self._rem_t(bar.datetime, p['pmcc_expiry'])
        curr = option_pricing.call_price(S, p['pmcc_k'], T, self.risk_free_rate, p['pmcc_iv'], self.codeId)
        return curr - p['pmcc_prem']

    def _stock_leg_close(self, p: dict, bar: BarData, close_reason: str, stock_exit: float) -> None:
        """CC phase 结束时平掉这份 long call（不管什么原因导致结束）。"""
        T    = self._rem_t(bar.datetime, p['pmcc_expiry'])
        curr = option_pricing.call_price(stock_exit, p['pmcc_k'], T, self.risk_free_rate, p['pmcc_iv'], self.codeId)
        emit_intent(self, 'pmcc_close', p, bar, right='call', leg_name='PMCC_stock',
                    old_k=p['pmcc_k'], old_prem=p['pmcc_prem'], close_price=curr,
                    close_reason=close_reason)

    def _stock_leg_maintain(self, p: dict, bar: BarData) -> None:
        """剩余天数 ≤ pmcc_roll_before_days 时主动展期，不管当前有没有持有
        short call（长腿的生命周期跟短腿独立）。"""
        if self.pmcc_roll_before_days <= 0:
            return
        days_left = (p['pmcc_expiry'] - bar.datetime).days
        if days_left > self.pmcc_roll_before_days:
            return
        self._roll_pmcc_leg(p, bar)

    def _roll_pmcc_leg(self, p: dict, bar: BarData) -> None:
        """平掉即将到期的旧 long call（浮盈浮亏计入 prev_premiums，跟 short call
        滚仓时的记账方式一致），在当前价位重新买入一份全新的 pmcc_days 天期
        long call 接续（复用 _stock_leg_open）。"""
        S        = bar.close_price
        T_old    = self._rem_t(bar.datetime, p['pmcc_expiry'])
        curr_old = option_pricing.call_price(
            S, p['pmcc_k'], T_old, self.risk_free_rate, p['pmcc_iv'], self.codeId)
        realized = curr_old - p['pmcc_prem']
        p['prev_premiums'] += realized
        old_k, old_expiry = p['pmcc_k'], p['pmcc_expiry']
        emit_intent(self, 'pmcc_roll_close', p, bar, right='call', leg_name='PMCC_stock',
                    old_k=old_k, old_prem=p['pmcc_prem'], close_price=curr_old)
        self.logger.info(
            f"[{bar.datetime}] PMCC到期前主动展期: 平旧leg k={old_k} exp={old_expiry.date()} "
            f"curr={curr_old:.4f} realized={realized:.4f}"
        )
        self._stock_leg_open(p, bar)

    def _extra_record_fields(self, p: dict) -> dict:
        return {'pmcc_k': p.get('pmcc_k'), 'pmcc_prem': p.get('pmcc_prem'),
                'pmcc_expiry': p.get('pmcc_expiry')}
