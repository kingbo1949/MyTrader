import logging
import uuid
from typing import Dict, List, Any, Optional
from datetime import datetime

# 内部模块导入
from qib_trader.core.models import BarData, OrderData, TradeData, Direction


class BacktestEngine:
    """
    商业级事件驱动回测引擎 (重构版)
    核心改进：支持策略染色、独立仓位追踪与多策略广播过滤。
    """

    def __init__(self):
        self.strategy_pool: List[Any] = []
        # [修改] 订单池，key 为 order_id
        self.active_orders: Dict[str, OrderData] = {}

        # 账户统计
        self.balance: float = 100000.0  # 初始资金
        self.total_trades: List[TradeData] = []

        # 缓存行情数据，用于撮合
        self.current_bar: Optional[BarData] = None
        self.logger = logging.getLogger("Engine.Backtest")

    def add_strategy(self, strategy_instance: Any) -> None:
        """
        向策略池添加策略实例。
        """
        self.strategy_pool.append(strategy_instance)

    def send_order(self, strategy_name: str, symbol: str, direction: Direction,
                   price: float, volume: float, order_type: str) -> str:
        """
        策略调用接口：模拟下单。
        [修改] 接收 strategy_name 并存入 OrderData，实现订单染色。
        """
        # 使用 uuid 生成更可靠的唯一 ID
        order_id = f"ord_{uuid.uuid4().hex[:8]}"

        order = OrderData(
            order_id=order_id,
            strategy_name=strategy_name,  # [核心] 标记订单所属策略
            symbol=symbol,
            direction=direction,
            price=price,
            volume=volume
        )

        self.active_orders[order_id] = order
        self.logger.debug(f"订单已记录: {order_id} | 策略: {strategy_name} | {direction} {volume} @ {price}")
        return order_id

    def run_backtest(self, bars: List[BarData]) -> None:
        """
        核心回测循环
        """
        if not bars:
            self.logger.warning("没有可用的 Bar 数据，回测停止。")
            return

        self.logger.info(f"开始回测，数据总量: {len(bars)} 条")

        # 1. 触发策略初始化
        for strategy in self.strategy_pool:
            strategy.on_init()
            strategy.on_start()

        # 2. 遍历时间轴 (Bar-by-Bar)
        for bar in bars:
            self.current_bar = bar

            # A. 撮合当前未成交订单 (使用当前 Bar 模拟撮合)
            self._cross_orders()

            # B. 驱动策略逻辑 (广播行情)
            for strategy in self.strategy_pool:
                # 策略内部会自动根据 symbol 和 interval 进行过滤
                strategy.on_bar(bar)

        # 3. 停止所有策略
        for strategy in self.strategy_pool:
            strategy.on_stop()

        self.logger.info("回测结束，正在计算统计结果...")

    def _cross_orders(self) -> None:
        """
        简易撮合器。
        [修改] 生成 TradeData 时必须透传 strategy_name，确保策略持仓独立。
        """
        if not self.current_bar:
            return

        finished_orders = []

        for order_id, order in self.active_orders.items():
            can_cross = False

            # 简单的撮合逻辑：如果价格在当前 Bar 范围内，则按报单价成交
            match order.direction:
                case Direction.LONG:
                    if self.current_bar.low_price <= order.price:
                        can_cross = True
                case Direction.SHORT:
                    if self.current_bar.high_price >= order.price:
                        can_cross = True

            if can_cross:
                # [核心] 构造 TradeData，必须携带原始订单的 strategy_name
                trade = TradeData(
                    symbol=order.symbol,
                    order_id=order.order_id,
                    trade_id=f"trd_{uuid.uuid4().hex[:8]}",
                    strategy_name=order.strategy_name,  # [核心] 路由标签
                    direction=order.direction,
                    price=order.price,
                    volume=order.volume,
                    datetime=self.current_bar.datetime
                )

                self.total_trades.append(trade)

                # [修改] 广播成交信号给策略池。
                # 策略会根据 trade.strategy_name 进行过滤更新。
                for strategy in self.strategy_pool:
                    strategy.on_trade(trade)

                finished_orders.append(order_id)

        # 清理已成交订单
        for oid in finished_orders:
            del self.active_orders[oid]