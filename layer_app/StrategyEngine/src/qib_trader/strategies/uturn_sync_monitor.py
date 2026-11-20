"""
uturn_sync_monitor —— 监控用"策略"：不下单，只在NQ与ES同一根bar上同时命中
UTurnBottom（或UTurnTop）时打印提醒。完全借用`LiveEngine`已有的sync_signals/
tracker机制（跟`UTurnBSync_*`系列真实策略用的是同一套配置写法），本类自己
不实现任何跨品种信号判断逻辑。

signal_direction='bottom'时，直接读`LiveEngine`自动建好的'sync'tracker写回
的`self._sync_timestamps`（`live/engine.py:_build_trackers`，只要
`sync_signals`非空就总会建一个`direction='bottom'`的tracker）。

signal_direction='top'时，`live/engine.py`里没有现成的"sync+top"tracker可以
直接用——`_build_trackers`里`direction='top'`的tracker只在dip_reopen那条
分支里建。本类通过在构造参数里顺带设置
`dip_reopen_level=1`+`profit_roll_wait_reopen=True`+`dip_reopen_sync=True`
（默认），"借用"这条分支——`dip_reopen_sync=True`时它用的信号集合就是
`sync_signals`本身，跟真正的滚仓/重开仓逻辑完全无关，这里只是借用它"用
`sync_signals`建一个`direction='top'`的tracker"这个副作用，省掉重新实现
一遍同样的tracker构建代码。`self._dip_reopen_timestamps`因此会被写入"NQ+ES
同一根bar都命中UTurnTop"的时间戳，跟原本"滚仓后等UTurnTop信号才重新开仓"
的业务语义完全无关。

`uturn_btm_level`固定给Level 1（跟用户确认的一致，单bar`is_uturn`+`div_type`
判定，不看多bar窗口）。
"""
from qib_trader.core.models import BarData
from qib_trader.strategies.base import Strategy


class uturn_sync_monitor(Strategy):
    """NQ+ES UTurn同步监控：不下单，只在logic()里打印提醒。"""

    initial_capital: float = 100_000.0   # Strategy基类要求的属性，监控用不到

    signal_direction:   str = 'bottom'   # 'bottom'或'top'，决定读哪个时间戳集合
    uturn_btm_level:     int = 1         # 固定Level 1
    multi_uturn_window:  int = 50        # Level 1不用这个窗口，保留只是满足_build_trackers签名

    def on_init(self) -> None:
        self._sync_timestamps: set = set()
        self._dip_reopen_timestamps: set = set()
        self._last_alert_dt = None   # 防重复打印（理论上不会重复，留作保险）
        self.alerts: list[str] = []  # 供日志打印用，跟文件日志并行
        self.last_bar: BarData | None = None   # 最近一次喂给logic()的bar，供GUI状态面板读取
        self.is_active_now: bool = False       # 当前bar是否命中同步信号，供GUI状态面板实时读取

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False

    def logic(self, bar: BarData) -> None:
        ts_set = self._dip_reopen_timestamps if self.signal_direction == 'top' else self._sync_timestamps
        self.last_bar = bar
        self.is_active_now = bar.datetime in ts_set
        if not self.is_active_now or bar.datetime == self._last_alert_dt:
            return
        self._last_alert_dt = bar.datetime
        kind = 'UTurnTop' if self.signal_direction == 'top' else 'UTurnBottom'
        msg = f"[{bar.interval.value}] {bar.datetime}  NQ与ES同时命中{kind}"
        self.logger.info(msg)
        self.alerts.append(msg)
