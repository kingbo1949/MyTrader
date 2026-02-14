4# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

MyTrader 是一个 C++17 多平台量化交易系统，包含行情数据处理、技术分析、图表可视化和策略交易功能。支持 macOS 和 Windows 双平台。

## 构建命令

```bash
# macOS 配置与构建
cmake --preset macos
cmake --build cmake-build-macos

# Windows 配置与构建 (PowerShell)
cmake --preset windows
cmake --build cmake-build-win

# Windows 安装到指定目录
cmake --install cmake-build-win --config Debug --prefix C:\MyCmakeApps
```

构建生成器为 Ninja，CMake 最低版本要求 4.0。构建产物输出到 `cmake-build-<平台>/bin/`。

## 分层架构

项目采用严格的分层架构，依赖方向为自上而下：

```
layer_app              ← 最终应用程序（GUI 和命令行工具）
layer_factory_app      ← 应用级业务逻辑（策略算子、策略实现）
layer_factory_base1    ← 分析层（成交分析、回调、模拟器、统一接口）
layer_factory_base0    ← 基础设施层（日志、数据库、全局共享、JSON、监控）
layer_lib              ← 核心库（BerkeleyDB封装、系统定义、Ice编译、工具库）
```

### 核心模块说明

- **layer_lib/IBSysDefine** - 全局系统定义和基础数据结构
- **layer_lib/BdbLib** - Berkeley DB 数据库封装（静态库）
- **layer_lib/IceCompiler** - ZeroC Ice 中间件 Slice 文件编译
- **layer_lib/ToolLib** - 通用工具（日志、数学计算、线程）
- **layer_factory_base0/Factory_QDatabase** - 行情数据库接口，通过 Ice 中间件通信
- **layer_factory_base0/Factory_IBGlobalShare** - 全局变量管理、时区/交易日、坐标转换
- **layer_factory_app/Factory_StOperator** - 策略算子（含 Break4Week、UTurn 实现）
- **layer_factory_app/Factory_Strategys** - 策略实现（开仓、平仓）

### 应用程序

- **AnalystGui** - Qt6 图表分析 GUI（K线、MACD、ATR、EMA 等技术指标）
- **QIBDatabase** - 数据库管理服务（K线/Tick/指标计算与存储）
- **QAnalyst** - Qt6 行情分析工具
- **QTransKLineFormIB** - Tick 转 K线、数据清洗修复
- **QuoteGenerator** - 行情数据生成
- **StLoader** - 策略加载器
- **JsonCsvTransfer** - JSON/CSV 配置格式互转

## 依赖库

| 库 | 用途 |
|---|---|
| Berkeley DB 6.2 | 持久化存储 |
| Boost 1.90.0 | filesystem, regex, locale（静态链接）|
| ZeroC Ice 3.7.9 | 分布式中间件/RPC |
| JsonCpp 1.9.5 | JSON 解析 |
| spdlog | 日志（header-only）|
| concurrentqueue | 无锁队列（header-only）|
| Qt6 | GUI 框架（Widgets, PrintSupport）|
| iconv | 字符编码转换（仅非 Windows）|
| Intel RDFP Math | 十进制数学运算（仅 Windows）|

macOS 下第三方库安装在 `/Users/fantang/local/` 目录；Windows 下位于项目内 `ThirdParty/` 目录。路径通过 `CMakePresets.json` 配置。

## CMake 模块

`cmake/` 目录下的关键模块：
- `build_options.cmake` - 统一编译标准（C++17）和平台宏定义（`ICE_CPP11_MAPPING` 等）
- `Dependencies.cmake` - 创建所有外部 imported targets
- `setup_runtime.cmake` - 提供 `setup_makedir()` 和 `setup_cp()` 函数，用于构建后创建运行目录和拷贝配置文件

## 构建约定

- Factory 模块通常构建为**共享库**（SHARED），使用 `*_EXPORTS` 宏和 hidden visibility
- layer_lib 中的基础库通常为**静态库**（STATIC）
- 每个应用程序在 `bin/run/<APP_NAME>/` 下有独立的运行时目录结构，包含 `config.server`、`db/`（JSON配置）、`log/` 子目录
- 命名空间别名统一使用 `MyTrader::` 前缀

## 运行时配置

- `config/config.server` - Ice 中间件发现和数据库端点配置
- `config/db/*.json` - 业务配置（合约、交易时段、策略参数、时区模拟）
- `config/csv/*.csv` - 同名 JSON 的 CSV 格式版本
