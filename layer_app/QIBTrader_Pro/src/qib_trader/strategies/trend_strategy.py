from qib_trader.strategies.base import Strategy
from qib_trader.core.models import BarData, Direction


class DoubleMAStrategy(Strategy):
    """
    双均线简单测试策略 (重构版)
    集成基类周期校验、持仓独立及方向防火墙功能。
    """

    def on_init(self) -> None:
        """
        初始化策略。
        [注意] self.fast_window 和 self.slow_window 已由基类 _update_params 自动注入。
        """
        self.logger.info(f"初始化策略 {self.name}，参数: Fast={self.fast_window}, Slow={self.slow_window}")
        self.history: list[float] = []

    def on_start(self) -> None:
        """启动逻辑"""
        self.active = True
        self.logger.info(f"{self.name} 启动")

    def on_stop(self) -> None:
        """停止逻辑"""
        self.active = False
        self.logger.info(f"{self.name} 停止")

    def logic(self, bar: BarData) -> None:
        """
        核心交易逻辑。
        [修改] 基类 on_bar 已处理品种/周期过滤，此处专注于信号计算与下单。
        """
        # 1. 更新历史价格序列
        self.history.append(bar.close_price)

        # 2. 需要累积数据足够多才能开始计算均线
        if len(self.history) < self.slow_window:
            return

        # 3. 计算简单均线 (SMA)
        fast_ma = sum(self.history[-self.fast_window:]) / self.fast_window
        slow_ma = sum(self.history[-self.slow_window:]) / self.slow_window

        # 4. 简单的金叉死叉逻辑
        # [注意] self.buy 和 self.sell 已由基类封装了 direction_limit 防火墙。

        # 金叉逻辑：快线上穿慢线
        if fast_ma > slow_ma:
            # 如果当前无多头持仓 (pos <= 0)，尝试买入
            if self.pos <= 0:
                self.logger.info(f"[{self.name}] 产生金叉信号，执行买入")
                self.buy(bar.close_price, 1)

        # 死叉逻辑：快线下穿慢线
        elif fast_ma < slow_ma:
            # 如果当前无空头持仓 (pos >= 0)，尝试卖出
            if self.pos >= 0:
                self.logger.info(f"[{self.name}] 产生死叉信号，执行卖出")
                self.sell(bar.close_price, 1)

    # [覆盖] 默认的 on_bar 已由基类处理，此处无需重写，基类会自动调用 logic()。