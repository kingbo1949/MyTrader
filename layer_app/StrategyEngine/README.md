# StrategyEngine

MyTrader 系统的 Python 策略引擎，负责策略回测、参数优化和滚动 Walk-Forward Optimization。

## 快速开始

```bash
pip install -r requirements.txt

# 回测（需要 QIBDatabase C++ 服务在线）
python scripts/main_backtest.py

# 参数优化
python scripts/optimizer.py

# 滚动 WFO
python scripts/wfo.py
```

## 目录结构

```
StrategyEngine/
├── scripts/
│   ├── main_backtest.py     # 回测入口：多策略、多品种统一时间轴
│   ├── optimizer.py         # 参数优化：多进程网格搜索，按收益风险比排序
│   └── wfo.py               # Walk-Forward Optimization：IS 网格搜索 + OOS 验证
├── src/qib_trader/
│   ├── api/converters.py    # IceConverter：ICE 结构体 ↔ BarData 转换
│   ├── core/
│   │   ├── models.py        # BarData、OrderData、TradeData、Direction、Interval
│   │   ├── broker.py        # SimBroker 单例：市价单即时撮合
│   │   ├── engine.py        # BacktestEngine：逐 Bar 事件循环
│   │   ├── strategy_pool.py # StrategyPool 单例：多策略生命周期管理
│   │   └── factory_strategy.py  # FactoryStrategy：按类名实例化策略
│   ├── strategies/
│   │   ├── base.py          # Strategy 抽象基类
│   │   ├── uturn_bottom.py  # UTurnBottom：底部 UTurn + Bull Call Spread 期权策略
│   │   └── break_4week.py   # Break4Week：四周突破策略
│   └── utils/
│       ├── factory_ice.py   # ICE 连接单例，分批拉取增强 K 线
│       ├── stats.py         # PerformanceAnalyzer：绩效统计（支持期权/期货双模式）
│       ├── config_loader.py # YAML 配置读取
│       └── log_setup.py     # 日志初始化
├── config/
│   ├── settings.yaml        # 合约乘数、环境配置
│   ├── strategies.yaml      # 回测策略列表与参数
│   ├── optimization.yaml    # 参数优化网格配置
│   └── wfo.yaml             # WFO 窗口与网格配置
├── ice/                     # C++ Slice 自动生成的 Python 绑定（勿手动修改）
└── output/                  # 回测导出的成交记录 CSV
```

## 配置说明

### strategies.yaml

```yaml
strategies:
  - class: "UTurnBottom"     # 策略类名（须与 Python 类名完全一致）
    enable: true
    code_id: "NQ"
    interval: "M15"
    direction: "LONG"
    start_time: "2024-01-05 00:00:00"
    end_time:   "2026-04-01 00:00:00"
    params:
      initial_capital: 100000
      iv: 0.24
      # ... 其他策略参数
```

### optimization.yaml

```yaml
optimizations:
  - class: "UTurnBottom"
    enable: true
    code_id: "NQ"
    interval: "M15"
    direction: "LONG"
    start_time: "2024-01-05 00:00:00"
    end_time:   "2026-01-01 00:00:00"
    grid:
      dif_entry:   [-20, -30]   # 参数搜索网格（笛卡尔积）
      dif_profit:  [20, 30, 40]
```

### wfo.yaml

```yaml
wfo_tasks:
  - class: "UTurnBottom"
    enable: true
    is_months:   9       # In-Sample 窗口长度（月）
    oos_months:  3       # Out-of-Sample 步长（月）
    optimize_by: "recovery_factor"
    grid:
      # 同 optimization.yaml 的 grid 格式
```

## 新增策略

1. 在 `src/qib_trader/strategies/` 新建 `.py` 文件
2. 继承 `Strategy`，实现 `on_init`、`on_start`、`on_stop`、`logic(bar)` 四个方法
3. 期权策略在类级别声明 `options_mode = True` 和 `initial_capital`
4. **在 `on_stop()` 中强制平仓所有未了结持仓**（否则回测截止时持仓被忽略，指标失真）
5. 无需手动注册，`strategies/__init__.py` 自动发现

## 绩效指标说明

| 指标 | 计算方式 |
|------|----------|
| 收益风险比 | `total_return% ÷ abs(max_drawdown%)` |
| 最大回撤 | 百分比及对应发生时间 |
| 夏普比率 | 年化 `(mean_ret / std_ret) × √252` |
| 胜率 | 盈利轮次 ÷ 总轮次（round-trip FIFO 配对） |

期权模式（`options_mode=True`）使用 FIFO 实现损益构建净值曲线，不盯市。
