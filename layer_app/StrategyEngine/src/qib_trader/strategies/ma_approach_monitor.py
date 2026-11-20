"""
ma_approach_monitor —— 监控用"策略"：不下单，只在价格逼近数据库里已经实时
算好的均线（ma5/ma20/ma60/ma200，见`core/models.py`的`BarData`）时打印提醒。

均线值直接读当前bar自带的`bar.ma{period}`，不取上一根bar的均线——用户已
明确说明不需要刻意避免未来数据（这些均线是数据库端实时算好写回来的，本身
不是用这根bar自己的收盘价现算出来的，所以直接用当前值不构成"用了未来数据"）。

判定规则（阈值固定±threshold_points点，默认30）：
  上一根bar收盘价 < 均线-threshold_points ，本根bar收盘价 ≥ 均线-threshold_points
    → "从下方上涨进入均线-30"
  上一根bar收盘价 > 均线+threshold_points ，本根bar收盘价 ≤ 均线+threshold_points
    → "从上方下跌进入均线+30"
这是单纯的阈值穿越判断，不是"进入[均线-30,均线+30]这整个区间后要一直停留在
里面"，价格继续往均线本身靠近或穿过均线不影响这一次提醒是否触发。

`periods`参数控制这个实例要监控哪些周期的均线（现在所有周期统一传
`periods=(5,20,60,200)`，包括M1）。一个(code_id,interval)用一个实例即可，
内部循环`periods`挨个检查，不需要每个均线周期单开一个实例。

`last_bar`：每次`logic()`都更新为最近一次喂进来的bar，供GUI状态面板实时
读取`close_price`/`ma{period}`算当前偏离值——跟下面的"穿越阈值才打印提醒"
是两套并行的东西：`alerts`/`logger.info`只在穿越那一刻触发（给日志文件用），
`last_bar`则是任何时候都能拿到的"当前快照"（给GUI用）。
"""
from qib_trader.core.models import BarData
from qib_trader.strategies.base import Strategy


class ma_approach_monitor(Strategy):
    """价格逼近均线监控：不下单，只在logic()里打印提醒。"""

    initial_capital:  float = 100_000.0   # Strategy基类要求的属性，监控用不到
    threshold_points: float = 30.0        # "逼近"阈值：均线±这么多点
    periods:          tuple = (5, 20, 60, 200)   # 要监控哪些周期的均线；M1实例应传(20,60,200)

    def on_init(self) -> None:
        self._prev_close: float | None = None
        self.alerts: list[str] = []          # 供日志打印用，跟文件日志并行
        self.last_bar: BarData | None = None  # 最近一次喂给logic()的bar，供GUI状态面板读取

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False

    def logic(self, bar: BarData) -> None:
        self.last_bar = bar
        if self._prev_close is not None:
            for period in self.periods:
                ma_value = getattr(bar, f"ma{period}", 0.0)
                if ma_value <= 0:
                    continue   # 数据库还没算出来这根均线（比如刚启动、历史不够），跳过不拦截
                lower = ma_value - self.threshold_points
                upper = ma_value + self.threshold_points
                if self._prev_close < lower <= bar.close_price:
                    self._alert(bar, period, ma_value, rising=True)
                elif self._prev_close > upper >= bar.close_price:
                    self._alert(bar, period, ma_value, rising=False)
        self._prev_close = bar.close_price

    def _alert(self, bar: BarData, period: int, ma_value: float, rising: bool) -> None:
        if rising:
            desc = f"上涨进入MA{period}-{self.threshold_points:.0f}"
        else:
            desc = f"下跌进入MA{period}+{self.threshold_points:.0f}"
        msg = f"{desc}（MA{period}={ma_value:.2f}, close={bar.close_price:.2f}）"
        self.logger.info(f"[{bar.codeId} {bar.interval.value}] {bar.datetime}  {msg}  <obj={id(self):#x}>")
        self.alerts.append(msg)
