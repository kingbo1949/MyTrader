from abc import ABC, abstractmethod
from typing import Dict, Any, List, Optional
import logging

# [修改] 引入核心数据模型，确保包含 Interval 枚举
from qib_trader.core.models import BarData, Direction, OrderData, TradeData, Interval
from qib_trader.core.order_sender import OrderSender


class Strategy(ABC):
    """
    商业级策略基类 (重构版)
    集成了周期校验、仓位独立逻辑以及严格的方向防火墙。
    """

    def __init__(self, name: str, symbol: str, interval: str, order_sender: OrderSender, direction_limit: str, params: Dict[str, Any]):
        self.name: str = name  # 策略实例唯一标识 (用于订单染色)
        self.symbol: str = symbol  # 标的代码 (如 NQ, ES)
        self.order_sender: OrderSender = order_sender # 发单器

        # [新增] 策略预设周期与方向限制
        self.interval: str = interval  # 期望周期字符串，如 "M15"
        self.direction_limit: str = direction_limit  # 严格限制为 "LONG" 或 "SHORT"

        self.params: Dict[str, Any] = params

        # 策略状态
        self.active: bool = False  # 策略是否正在运行
        self.pos: float = 0.0  # [核心] 策略独立的逻辑持仓

        # 日志配置
        self.logger = logging.getLogger(f"Strategy.{self.name}")

        # 自动参数注入
        self._update_params(params)

    def _update_params(self, params: Dict[str, Any]) -> None:
        """利用反射机制将配置文件的参数同步到实例属性"""
        for key, value in params.items():
            setattr(self, key, value)

    # --- 策略生命周期钩子 ---

    @abstractmethod
    def on_init(self) -> None:
        """初始化逻辑"""
        pass

    @abstractmethod
    def on_start(self) -> None:
        """启动逻辑"""
        self.active = True
        self.logger.info(f"策略 {self.name} 已启动")

    @abstractmethod
    def on_stop(self) -> None:
        """停止逻辑"""
        self.active = False
        self.logger.info(f"策略 {self.name} 已停止")

    # --- 数据驱动钩子 ---

    def on_bar(self, bar: BarData) -> None:
        """
        K线更新回调。
        [修改] 增加了品种与周期的双重过滤防火墙。
        """
        # 1. 品种校验
        if bar.symbol != self.symbol:
            return

        # 2. 周期校验：防止错误频率的数据驱动逻辑
        # 比较 Bar 的枚举值与其配置的字符串值
        if bar.interval.value != self.interval:
            return

        # 3. 只有校验通过才执行核心逻辑
        self.logic(bar)

    @abstractmethod
    def logic(self, bar: BarData) -> None:
        """
        策略核心逻辑。子类应重写此方法而非 on_bar。
        """
        pass

    # --- 交易反馈钩子 ---

    def on_order(self, order: OrderData) -> None:
        """订单状态变化回调"""
        pass

    def on_trade(self, trade: TradeData) -> None:
        """
        成交细节回调。
        [修改] 增加了 strategy_name 校验，确保仓位逻辑独立。
        """
        # [核心] 只处理属于本策略实例的成交
        if trade.strategy_name != self.name:
            return

        # 利用 3.10 match-case 更新内部逻辑持仓
        match trade.direction:
            case Direction.LONG:
                self.pos += trade.volume
            case Direction.SHORT:
                self.pos -= trade.volume

        self.logger.info(f"[{self.name}] 逻辑持仓更新: {self.pos} | 成交: {trade.direction} {trade.volume}")

    # --- 交易指令接口 (带方向防火墙) ---

    def buy(self, price: float, volume: float, order_type: str = "LMT") -> str:
        """
        买入操作（开多或平空）。
        [修改] 增加了对 SHORT_ONLY 策略开多的拦截。
        """
        # 如果是空头策略且当前无空头仓位，禁止买入开多
        if self.direction_limit == "SHORT" and self.pos >= 0:
            self.logger.warning(f"检测到方向违规：{self.name} 为 SHORT 策略，拒绝开多订单")
            return ""

        return self._send_order(Direction.LONG, price, volume, order_type)

    def sell(self, price: float, volume: float, order_type: str = "LMT") -> str:
        """
        卖出操作（开空或平多）。
        [修改] 增加了对 LONG_ONLY 策略开空的拦截。
        """
        # 如果是多头策略且当前无多头仓位，禁止卖出开空
        if self.direction_limit == "LONG" and self.pos <= 0:
            self.logger.warning(f"检测到方向违规：{self.name} 为 LONG 策略，拒绝开空订单")
            return ""

        return self._send_order(Direction.SHORT, price, volume, order_type)

    def _send_order(self, direction: Direction, price: float, volume: float, order_type: str) -> str:
        """
        内部发单逻辑。
        [修改] 此处不实现逻辑，由 Engine 运行时注入真正的发单实现。
        """
        self.order_sender.send(direction, price, volume, order_type)
        return ""