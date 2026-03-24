# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 模块定位

`StrategyEngine` 是 MyTrader 系统的 Python 策略引擎，负责策略回测、参数优化和滚动 WFO（Walk-Forward Optimization）。
通过 ZeroC Ice Python 绑定连接 C++ 后端（`QIBDatabase` 服务），获取含技术指标的增强 K 线数据。
C++ 层不在本目录中修改。

## 运行命令

```bash
# 安装依赖
pip install -r requirements.txt

# 运行回测（需要 QIBDatabase C++ 服务在线）
cd layer_app/StrategyEngine
python scripts/main_backtest.py

# 运行参数优化（多进程，按收益风险比排序）
python scripts/optimizer.py

# 运行滚动 Walk-Forward Optimization
python scripts/wfo.py
```

## 架构设计

### 数据流

```
C++ QIBDatabase 服务
    ↓ ZeroC Ice RPC (tcp -h 10.0.0.64 -p 10000)
FactoryIce.get_enriched_klines_loop()  # 分批拉取（含 MACD/ATR/UTurn 等技术指标）
    ↓
IceConverter.enriched_df_to_bars()    # ICE 结构体 → List[BarData]
    ↓
BacktestEngine.run_backtest()         # 事件驱动逐 Bar 回测
    ↓  broker.on_bar() 撮合 → pool.on_bar() 驱动策略
SimBroker                             # 简易撮合器，市价单即时成交
    ↓
PerformanceAnalyzer / stats.py        # 绩效统计与报告
```

### 核心层次

- **`utils/factory_ice.py`** - ICE 连接单例，`get_enriched_klines_loop()` 循环分批拉取含指标的 K 线
- **`api/converters.py`** - `IceConverter`：ICE 结构体 → DataFrame/BarData 转换
- **`core/models.py`** - 核心数据类：`BarData`（含 `dif`、`div_type`、`is_uturn`、`prev_day_*` 等技术字段）、`OrderData`、`TradeData`、`Direction`、`Interval`
- **`core/broker.py`** - `SimBroker`：单例撮合器，市价单即时成交；`make_broker()`/`get_broker()`/`reset_broker()` 管理生命周期
- **`core/strategy_pool.py`** - `StrategyPool` 单例：聚合多策略生命周期；`make_pool()`/`get_pool()`/`reset_pool()`
- **`core/engine.py`** - `BacktestEngine`：`broker.on_bar()` 先撮合上一 bar 的委托，`pool.on_bar()` 再驱动策略产生新委托，最后调用 `pool.on_stop()` 触发强制平仓
- **`core/factory_strategy.py`** - `FactoryStrategy.create_strategy(conf)`：按 `class` 字段查找策略类并实例化
- **`strategies/base.py`** - `Strategy` 抽象基类：`initial_capital=200_000`、`options_mode=False` 类级别默认，`_update_params()` 从 params dict 覆盖属性；`_send_order()` 直连 `SimBroker.on_order()`
- **`utils/stats.py`** - `PerformanceAnalyzer`：支持期货盯市模式和期权实现损益模式（`options_mode`）；`recovery_factor = total_return% / abs(max_dd_pct%)`

### 策略注册机制

`strategies/__init__.py` 在包导入时自动扫描 `strategies/` 目录下所有继承 `Strategy` 的具体类，注册到 `STRATEGY_MAP`。**新策略只需放入 `strategies/` 目录并继承 `Strategy`，无需手动注册。**

### 期权策略（options_mode）

当策略类设置 `options_mode = True` 时，`PerformanceAnalyzer` 使用 FIFO 实现损益构建净值曲线（忽略持仓盯市）。每个 spread 仅产生 **2 笔成交**：开仓一笔 `LONG @ net_debit`，平仓一笔 `SHORT @ net_debit + net_pnl`，价格单位为**期权权利金点数**（非标的价格）。

### 多进程优化注意事项

`optimizer.py` 和 `wfo.py` 使用 `multiprocessing.Pool`，子进程工作函数（`_run_single_backtest` / `_run_single`）必须在每次任务开始时调用 `reset_pool()` 和 `reset_broker()` 清除单例状态，防止 worker 进程复用时状态污染。

## 配置驱动

| 文件 | 用途 |
|------|------|
| `config/settings.yaml` | 合约乘数（`multiplier`）、环境配置（`export_trades_csv`）、ICE 端口 |
| `config/strategies.yaml` | 策略列表：`class`、`code_id`、`interval`、`direction`、`start_time`、`end_time`、`params` |
| `config/optimization.yaml` | 优化任务：`grid` 定义参数搜索网格，多进程并行，按 `recovery_factor` 降序排名 |
| `config/wfo.yaml` | WFO 任务：`is_months`（IS 窗口）、`oos_months`（OOS 步长）、`optimize_by`、`grid` |

`enable: false` 的任务会被跳过。`class` 字段须与 Python 类名完全一致。

## 编写新策略

继承 `Strategy`，覆盖 `on_init`、`on_start`、`on_stop`、`logic(bar)` 四个方法。期权策略需在类级别声明 `options_mode = True` 和 `initial_capital = <值>`（覆盖基类默认 200,000）。在 `on_stop()` 中**强制平仓**所有未了结持仓，以避免回测截止时持仓被忽略导致指标失真。

```python
from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData

class MyStrategy(Strategy):
    initial_capital: float = 100_000.0  # 覆盖基类默认值

    def on_init(self) -> None:
        self._last_bar = None

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False
        # 在此强制平仓未了结持仓，使用 self._last_bar

    def logic(self, bar: BarData) -> None:
        self._last_bar = bar
        if self.pos == 0:
            self._send_order(Direction.LONG, bar.close_price, 1, "MKT")
```

## ICE 依赖说明

- `ice/` 目录下是 C++ Slice 文件自动生成的 Python 绑定，不要手动修改
- `import IBTrader` 使用 `ice/IBTrader/__init__.py`，需将 `ice/` 加入 `PYTHONPATH` 或在项目根目录运行
- ICE 服务地址在 `utils/factory_ice.py` 的 `_create_proxy()`（`10.0.0.64:10000`），回测时需要 C++ 服务在线
- `get_enriched_klines_loop()` 返回含 MACD（`dif`、`dea`）、UTurn 信号（`div_type`、`is_uturn`）、前日价格（`prev_day_close`、`prev_day_high`、`prev_day_low`）等字段的增强数据
