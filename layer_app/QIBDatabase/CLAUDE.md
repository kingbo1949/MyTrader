# QIBDatabase 开发规则

## 项目概述
实时行情数据处理引擎。通过 Ice RPC 接收 Tick 数据，转换为 6 个时间周期（S15/M1/M5/M15/M30/H1/D1）的 K 线，
计算 6 种技术指标（MA/VWMA/EMA/MACD/DivType/ATR），全部持久化存储。

## 架构
```
Ice RPC (CQDatabaseImp)
  → TickUpdator (Tick→KLine 转换，6 个周期)
  → TimerTask_UpdateIndex (1秒周期，增量计算指标)
  → Calculator_* (MA/EMA/VWMA/MACD/DivType/ATR 计算器)
  → DbTable_* (数据库表操作)
  → Env_IB (数据库环境，管理所有表)
```

## 数据库表
| 表 | 数据库文件 | Key 类型 | Value 类型 | 说明 |
|---|---|---|---|---|
| DbTable_KLine | kline_his.db | IQKey | IKLine | K 线 OHLCV |
| DbTable_TickHis | tick_his.db | string | ITick | Tick 历史 |
| DbTable_Macd | macd_his.db | IQKey | IMacdValue | MACD 指标 |
| DbTable_Average (MA) | ma_his.db | IQKey | IAvgValue | 简单均线 |
| DbTable_Average (VWMA) | vwma_his.db | IQKey | IAvgValue | 成交量加权均线 |
| DbTable_Average (EMA) | ema_his.db | IQKey | IAvgValue | 指数均线 |
| DbTable_DivType | divtype_his.db | IQKey | IDivTypeValue | 背离类型 |
| CDbTable_Atr | atr_his.db | IQKey | IAtrValue | ATR 指标 |

## Ice 数据结构
- IQKey = codeId (string) + ITimeType (enum: S15/M1/M5/M15/M30/H1/D1)
- IKLine: open, high, low, close, volume, time
- ITick: time, price(last), volume, codeId
- IMacdValue: emaShort, emaLong, dif, dea, macd, time
- IAvgValue: value, time（MA/EMA/VWMA 共用）
- IDivTypeValue: divType, time
- IAtrValue: atr, time

## 查询模式 (IQuery.byReqType)
- 0: 从末尾往前取 N 条
- 1: 取全部
- 2: 从指定时间往前取 N 条
- 3: 从指定时间往后取 N 条
- 4: 时间范围查询 [beginPos, endPos]

## 关键文件
- `src/Env_IB.h/cpp` - 数据库环境，管理所有 DbTable 实例
- `src/QDatabaseImp.h/cpp` - Ice RPC 接口实现，所有数据库操作的入口
- `src/TickUpdator.h/cpp` - Tick 转 KLine 核心逻辑
- `src/DbTable/` - 所有数据库表的封装
- `src/Calculator/` - 技术指标计算器
- `src/Factory.h/cpp` - 全局单例工厂

## 添加文件和目录规则
- 每次添加文件或者目录，最多2个，并且要讲解他们添加的用途

## 跨平台规则
- 代码必须在 Windows (x86_64) 和 macOS (x86_64/ARM64) 上正确执行


