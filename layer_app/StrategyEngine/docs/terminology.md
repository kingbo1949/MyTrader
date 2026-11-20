# 术语定义

## 简单UTurnBottom信号同步

**定义**：指定的多个品种（`sync_signals` 列表）在**同一根 bar（相同时间戳）**同时满足简单UTurnBottom条件：

- `bar.is_uturn == True`
- `bar.div_type in ('Bottom', 'BottomSub')`
- `abs(bar.dif) >= min_dif_abs`（每个品种各自的阈值）

所有品种均满足时，称"同步信号成立"。

**用途**：`UTurnBSync_CreditPutSpread` 策略以此为开仓触发条件，以提高信号质量、过滤单品种的噪声信号。

**实现方式**：在回测启动阶段（`on_start`）预先对每个同步品种计算满足条件的时间戳集合，取交集得到同步时间戳集合 `_sync_timestamps`；策略逐 bar 运行时只需做 `bar.datetime in _sync_timestamps` 的 O(1) 查找。
