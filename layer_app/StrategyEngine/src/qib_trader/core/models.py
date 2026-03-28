from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum, auto
from typing import Optional

# --- 1. 基础枚举定义 ---

CodeId: type[str] = str
StrategyId: type[str] = str
OrderId: type[str] = str
TradeId: type[str] = str




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
    codeId: CodeId
    datetime: datetime
    interval: Interval  # [新增] 周期字段，用于频率校验
    open_price: float
    high_price: float
    low_price: float
    close_price: float
    volume: float
    open_interest: float = 0.0
    # 技术指标
    dif: float = 0.0
    dea: float = 0.0
    macd: float = 0.0         # MACD 柱子值 = dif - dea
    div_type: str = ""        # "Normal"/"Bottom"/"BottomSub"/"Top"/"TopSub"
    is_uturn: bool = False
    atr: float = 0.0
    avg_atr: float = 0.0          # 最近14周期 ATR 均值
    # 上一日日线数据（仅对日内 bar 有效）
    prev_day_high: float = 0.0
    prev_day_low: float = 0.0
    prev_day_close: float = 0.0

# --- 3. 交易执行数据模型 ---

@dataclass(slots=True)
class OrderData:
    order_id: OrderId
    strategy_id: StrategyId
    codeId: CodeId
    direction: Direction
    price: float
    volume: float
    status: OrderStatus = OrderStatus.SUBMITTED
    datetime: datetime = field(default_factory=datetime.now)

@dataclass(frozen=True, slots=True)
class TradeData:
    codeId: CodeId
    order_id: OrderId
    trade_id: TradeId
    strategy_id: StrategyId
    direction: Direction
    price: float
    volume: float
    datetime: datetime


# --- 4. 集合类型别名 ---

BarDatas: type[list] = list[BarData]
OrderDatas: type[list] = list[OrderData]
TradeDatas: type[list] = list[TradeData]


# --- 5. 数据工具类 ---

class DataUtils:

    @staticmethod
    def print_bars(bars: BarDatas, title: str = "BarData") -> None:
        """将 BarData 列表以对齐表格形式打印，便于调试。"""
        if not bars:
            print(f"[{title}] 空列表")
            return
        print(f"\n{'─' * 80}")
        print(f"  {title}  ({len(bars)} 根 bar)")
        print(f"{'─' * 80}")
        print(f"{'datetime':<22} {'open':>10} {'high':>10} {'low':>10} {'close':>10} {'volume':>10}")
        print(f"{'─' * 80}")
        for b in bars:
            print(f"{str(b.datetime):<22} {b.open_price:>10.2f} {b.high_price:>10.2f}"
                  f" {b.low_price:>10.2f} {b.close_price:>10.2f} {b.volume:>10.0f}")
        print(f"{'─' * 80}\n")
