# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 模块定位

`QIBTrader_Pro` 是 MyTrader 系统的 Python 策略层，负责策略回测、参数优化和未来的实盘信号。通过 ZeroC Ice Python 绑定连接 C++ 后端（`QIBDatabase` 服务），C++ 层不在本目录中修改。

## 运行命令

```bash
# 安装依赖
pip install -r requirements.txt

# 运行回测（需要 QIBDatabase C++ 服务在线）
cd layer_app/QIBTrader_Pro
python scripts/main_backtest.py

# 运行参数优化（多进程，按收益风险比排序）
python scripts/optimizer.py
```

## 架构设计

### 数据流

```
C++ QIBDatabase 服务
    ↓ ZeroC Ice RPC (tcp -h 10.0.0.64 -p 10000)
FactoryIce.get_klines_loop()   # 分批拉取（每批4000条）
    ↓
IceConverter.klines_to_df()    # ICE 结构体 → Pandas DataFrame
IceConverter.df_to_bars()      # DataFrame → List[BarData]
    ↓
BacktestEngine.run_backtest()  # 事件驱动逐Bar回测
    ↓
PerformanceAnalyzer            # 绩效统计报告
```

### 核心层次

- **`utils/factory_ice.py`** - ICE 连接单例，`get_klines_loop()` 循环分批拉取历史数据
- **`api/converters.py`** - `IceConverter`：ICE 结构体 ↔ DataFrame/BarData 转换
- **`core/models.py`** - 核心数据类：`BarData`（含 `Interval` 枚举）、`OrderData`、`TradeData`、`Direction`
- **`core/engine.py`** - `BacktestEngine`：Bar-by-Bar 事件循环 + 简易撮合器
- **`core/order_sender.py`** - `OrderSender` 抽象接口，`BacktestSender`/`RealTradeSender` 两种实现，通过工厂函数 `make_order_sender()` 按 `settings.yaml` 的 `mode` 切换
- **`strategies/base.py`** - `Strategy` 抽象基类：含品种+周期双重过滤防火墙、订单染色、方向防火墙（`direction_limit`）
- **`strategies/__init__.py`** - `discover_strategies()` 自动扫描并注册策略类，`get_strategy_class()` 按类名获取

### 策略注册机制

`strategies/__init__.py` 在包导入时自动扫描 `strategies/` 目录下所有继承 `Strategy` 的具体类，注册到 `STRATEGY_MAP`。**新策略只需放入 `strategies/` 目录并继承 `Strategy`，无需手动注册。**

### 订单染色与多策略隔离

每个 `OrderData` 和 `TradeData` 携带 `strategy_name` 字段。引擎广播成交时，策略在 `on_trade()` 中只处理 `trade.strategy_name == self.name` 的成交，保证多策略并行时持仓逻辑完全独立。

## 配置驱动

| 文件 | 用途 |
|------|------|
| `config/settings.yaml` | 合约乘数、手续费、运行模式（backtest/real）、ICE 端口 |
| `config/strategies.yaml` | 策略池：每个任务指定 `class`、`symbol`、`interval`、`direction_limit`、`params` |
| `config/optimization.yaml` | 优化任务：`grid` 定义参数搜索网格 |

策略任务的 `class` 字段必须与 Python 类名完全一致（`discover_strategies` 以类名为 Key）。`enable: false` 的任务会被跳过。

## 编写新策略

继承 `Strategy`，实现 `on_init`、`on_start`、`on_stop`、`logic(bar)` 四个方法。在 `logic()` 中调用 `self.buy()` / `self.sell()`（含方向防火墙），不要直接调用 `_send_order()`。

```python
from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData

class MyStrategy(Strategy):
    def on_init(self): pass
    def on_start(self): super().on_start()
    def on_stop(self): super().on_stop()
    def logic(self, bar: BarData):
        # bar 已通过 symbol + interval 双重过滤
        if self.pos == 0:
            self.buy(bar.close_price, 1)
```

## ICE 依赖说明

- `ice/` 目录下是 C++ Slice 文件自动生成的 Python 绑定，不要手动修改
- `import IBTrader` 使用的是 `ice/IBTrader/__init__.py`，需要将 `ice/` 加入 `PYTHONPATH` 或在项目根目录运行
- ICE 服务地址硬编码在 `utils/factory_ice.py` 的 `_create_proxy()`（`10.0.0.64:10000`），回测时需要 C++ 服务在线
