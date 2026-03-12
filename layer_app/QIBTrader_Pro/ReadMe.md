## 项目目录结构
```
QIBTrader_Pro/
├── .gitignore
├── ReadMe.md
├── requirements.txt         # 依赖清单 核心依赖：pyyaml, pandas, numpy, matplotlib, ice-python
├── setup.py                 # 项目安装脚本
├── config/                  # # 配置文件中心：实现“代码与逻辑分离”的关键
│   ├── settings.yaml        # 全局静态配置：品种合约乘数(Multiplier)、手续费(Commission)、ICE 端口等
│   ├── strategies.yaml      # 回测任务配置：定义策略池、品种、周期及对应的类名(class)
│   └── optimization.yaml    # 优化任务配置：定义参数搜索网格(Grid)及性能目标
├── ice/                     # ICE 自动生成的 Python 绑定文件，通讯层：C++ 接口生成的 Python 绑定 (IBTrader/QDatabase)
│   ├── IBTrader/
│   │   └── __init__.py
│   ├── QDatabase_ice.py
│   └── QStruc_ice.py
├── src/                     # 核心源码包
│   └── qib_trader/          # 顶层包名
│       ├── __init__.py
│       ├── api/             # 数据转换层：将 C++ 结构体转换为高性能 Numpy/Pandas 容器
│       │   ├── __init__.py
│       │   ├── ice_client.py    # 封装与 C++ 的连接池
│       │   └── converters.py    # 负责将 ICE 结构体高效转换为 Numpy/Pandas
│       ├── core/                # 核心引擎：包含数据模型(models)、事件驱动引擎(engine)
│       │   ├── __init__.py
│       │   ├── models.py
│       │   ├── engine.py        # 回测调度引擎（处理事件循环）
│       │   ├── portfolio.py     # 组合管理、仓位跟踪、风控
│       │   └── account.py       # 虚拟账户（保证金计算、手续费）
│       ├── data/                # 数据访问抽象层 (DAL)
│       │   ├── __init__.py
│       │   ├── store.py         # 抽象 IQDatabase，支持从 ICE 或本地缓存读取
│       │   └── processor.py     # 特征工程、数据清洗、指标计算
│       ├── strategies/          # 策略逻辑池：包含基类(base)及具体策略实现
│       │   ├── __init__.py      # 策略工厂：通过 discover_strategies 自动扫描并注册策略类
│       │   ├── base.py          # 策略抽象基类
│       │   ├── break_4week.py   # 四周突破策略
│       │   ├── trend_strategy.py # 双均线趋势策略
│       │   └── collection/      # 策略实现类文件夹
│       └── utils/               # 支撑工具：配置加载、ICE Factory、回测统计分析(stats)
│           ├── __init__.py
│           ├── config_loader.py # YAML 配置文件加载器
│           ├── factory_ice.py   # 通讯器、代理等与ICE相关的工具
│           ├── stats.py         # 量化回测绩效分析器
│           └── tool.py          # 工具包
├── tests/                   # 单元测试与集成测试（与源代码并列）
│   ├── test_engine.py
│   └── test_converters.py
├── scripts/                 # 入口脚本
│   ├── __init__.py
│   ├── main_backtest.py     # 多策略并行/串行回测启动入口
│   ├── sync_data.py         # 离线数据同步脚本
│   └── optimizer.py         # 多进程参数优化工具，按“收益风险比”排序
└── logs/                    # 日志存储
```
