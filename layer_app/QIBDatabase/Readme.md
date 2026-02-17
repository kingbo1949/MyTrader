# QIBDatabase 接口设计

## 更新K线
- UpdateKLine 简单更新单笔K线，不更新指标，因为ICE的传输有规模限制，提供UpdateKLines接口意义不大

## 更新指标
- 指标更新有两种模式：Initialize/Recount模式 + Update模式，前者是全量更新更快，后者是增量更新更慢
- 提供RecountAllIndex接口，对应CCmd_RecountAllIndexRecountAllIndex对象，全量更新指标
- 提供UpdatAllIndexFromTimePos接口，对应CCmd_UpdateAllIndexFromTimePos对象，增量更新指标


## 接口使用场景
### 实时行情K线更新
- 接受到TICK，发送到tickupdator 
- 发送到tickupdator做出K线，直接把K线更新到数据库
- tickupdator判断K线如果是新生成的，属于急于更新指标数据，做出CCmd_UpdateAllIndexFromTimePos对象，直接加入线程池
- tickupdator判断K线如果不是新生成的，把KEY加入CTimerTask_UpdateIndex队列，CTimerTask_UpdateIndex队列中都是不急于更新的指标数据
- CTimerTask_UpdateIndex每隔1秒被唤醒，醒后检查线程池全部空闲，没有空闲就退出，有空闲就开始把队列做成CCmd_UpdateAllIndexFromTimePos对象，加入线程池

### CCmdUpdateDbFromIB QGenerator盘中定时从IB下载数据，已经被停用
### CSetup_QGenerator::QueryAndUpdateKline QGenerator启动时从IB下载数据
- 通过UpdateKLine把下载的K线全部更新
- 调用UpdatAllIndexFromTimePos接口并生成CCmd_UpdateAllIndexFromTimePos对象，加入线程池
- 线程池执行完毕，空闲之后，开始向IB订阅行情

### CDownloadData::UpdateDbKLineFromIB StLoader手动更新行情
- 通过UpdateKLine把下载的K线全部更新
- 调用UpdatAllIndexFromTimePos接口并生成CCmd_UpdateAllIndexFromTimePos对象，加入线程池

### QTransKLineFormIB 数据迁移时从文件更新大批K线数据
- 通过UpdateKLine把K线一个个简单加入
- 调用RecountAllIndex接口，它会生成CCmd_RecountAllIndexRecountAllIndex对象，加入线程池

### QTransKLineFormIB 更新指标
- 调用UpdatAllIndexFromTimePos接口，它会生成CCmd_UpdateAllIndexFromTimePos对象，加入线程池，把所有指标update
