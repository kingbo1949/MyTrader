from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum, auto
from typing import Optional

# --- 1. 基础枚举定义 ---

class Interval(Enum):
    """
    周期枚举定义
    [新增] 用于标识 K 线周期，确保策略只处理对应频率的数据。
    其字符串值与 factory_ice.py 中的 ITimeType 保持命名一致。
    """
    M1 = "M1"
    M5 = "M5"
    M15 = "M15"
    M30 = "M30"
    H1 = "H1"
    D1 = "D1"

class Direction(Enum):
    """
    交易方向枚举
    [修改] 根据最新讨论，方向仅保留 LONG 和 SHORT，不再设 BOTH。
    """
    LONG = "LONG"    # 多头
    SHORT = "SHORT"  # 空头
    FLAT = "FLAT"    # 平仓/无仓位

class OrderStatus(Enum):
    """订单状态枚举"""
    SUBMITTED = auto()  # 已提交
    TRADED = auto()     # 已成交
    CANCELLED = auto()  # 已撤销
    REJECTED = auto()   # 已拒绝

# --- 2. 核心行情数据模型 ---

@dataclass(frozen=True, slots=True)
class BarData:
    """
    K线数据模型
    [修改] 添加了 interval 字段，类型为 Interval 枚举。
    这允许策略在 on_bar 中校验输入数据的频率是否正确。
    """
    symbol: str
    datetime: datetime
    interval: Interval  # [新增] 周期字段，用于频率校验
    open_price: float
    high_price: float
    low_price: float
    close_price: float
    volume: float
    open_interest: float = 0.0

# --- 3. 交易执行数据模型 ---

@dataclass(slots=True)
class OrderData:
    """
    订单数据模型
    [修改] 添加了 strategy_name 字段。
    这是实现“仓位逻辑独立”的核心，用于标记该订单由哪个策略实例发出。
    """
    order_id: str
    strategy_name: str  # [新增] 策略唯一标识，实现订单染色
    symbol: str
    direction: Direction
    price: float
    volume: float
    status: OrderStatus = OrderStatus.SUBMITTED
    datetime: datetime = field(default_factory=datetime.now)

@dataclass(frozen=True, slots=True)
class TradeData:
    """
    成交明细数据模型
    [修改] 添加了 strategy_name 字段。
    当引擎广播成交时，策略根据此字段判断是否属于自己的成交，从而更新内部持仓。
    """
    symbol: str
    order_id: str
    trade_id: str
    strategy_name: str  # [新增] 所属策略名称，用于成交路由过滤
    direction: Direction
    price: float
    volume: float
    datetime: datetime