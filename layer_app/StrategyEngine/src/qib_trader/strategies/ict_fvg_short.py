"""
ict_fvg_short —— ICT Fair Value Gap（公允价值缺口）空头策略。是ict_fvg_long的
镜像版本（看跌缺口+MA下跌→开空），本策略只交易标的本身，不是期权策略；每笔
交易的生命周期严格限定在"形成信号的那一根bar"之内（要么intrabar止盈，要么该
bar收盘时平仓），不会跨bar持仓。

FVG识别（_detect_new_fvg）：
  每根新bar到达时，检查index-2/index-1/index这3根bar：若bar1.low > bar3.high，
  则[bar3.high, bar1.low]构成一个"看跌型"缺口。中间那根bar（bar2，造成
  这个缺口的位移bar）还必须是阴线（close<open，跟_bar_path里"阴线"的判定
  口径一致，跟多头版"必须阳线"是镜像要求）——要求这根位移bar本身也是下跌
  的，跟缺口的看跌方向一致，过滤"缺口形状对但中间bar其实在涨"这种杂音
  形态。这一步跟"两条MA斜率"是两道独立的方向过滤，前者在缺口形成时
  （_detect_new_fvg）就判断一次，后者在每次尝试开仓时（_check_entries）
  都重新判断一次（见下面enable_ma_filter的说明）。
  缺口下沿=bar3.high，上沿=bar1.low。止损改用固定价位数（stop_loss_ticks×
  tick_size），不再用中间bar（bar2）的开盘价——跟ict_fvg_long同样的理由：
  bar2开盘价跟入场价之间的距离不可控，不同缺口的实际止损风险差异很大；
  固定价位每笔风险一致，止损价=入场价+stop_loss_ticks×tick_size（空头
  止损在更高处），在开仓那一刻才算出来，不在缺口形成时就固定。
  所有"多少个点"的参数（target_ticks/min_gap_ticks/stop_loss_ticks）统一
  按"价位数×tick_size"换算成实际价格距离——跟ict_fvg_long同样的理由：NQ
  最小变动价位是0.25，股票是0.01，直接用原始价格差值在品种之间不可比，
  改成"价位数"换品种只需要改tick_size，不用重新调threshold数值本身。
  缺口宽度（upper-lower）小于min_gap_ticks×tick_size时直接忽略，不加入_active_fvgs
  追踪（太窄的缺口噪音大，容易一触即发又立刻被打穿，不值得开仓）。
  新形成的缺口要等到**下一根bar**才会被_check_entries检查到（在logic()里
  _check_entries先于_detect_new_fvg执行），避免"缺口刚形成、下沿正好等于本
  bar最高价"这种trivial自触发。

开仓条件（_check_entries）：
  两条MA（ma_period/ma2_period两个周期，各自独立配置，默认5和10）都必须
  是跌的，缺一不可（用当前bar**之前**已收盘的两个MA值比较斜率，不含
  当前这根信号bar，跟多头版同样的理由：entry_price假定按限价单挂在bar
  开始前、intrabar被动触发成交，用当前bar自己的收盘价算MA等于用了未来
  数据，之前版本有这个bug，已修正）。enable_ma_filter=False时直接跳过这整
  道门槛（_ma_falling恒返回True，两条MA都不看），只看缺口+价位条件，不再
  要求MA方向配合——跟_detect_new_fvg里bar2阴线/阳线的过滤是独立的两道关，
  这个开关只关掉MA那一道，bar2方向过滤始终生效，没有对应开关。
  + 价格从下往上突破某个未失效缺口的下沿，并且继续再涨entry_offset_ticks
  个价位（上一根bar收盘价<下沿，本bar最高价≥trigger_price，trigger_price
  =下沿+entry_offset_ticks×tick_size）→ 按trigger_price价位开空（假定能
  在这个价位精确成交，跟多头版的简化处理方式一致）。entry_offset_ticks=0
  时trigger_price就等于下沿，退回最初"一突破下沿就开空"的行为；调大这个
  值相当于要求价格在突破下沿之后再多走一段确认，才真正开仓，跟多头版
  entry_offset_ticks是完全镜像的手法（多头往下偏移，空头往上偏移）。同样，
  "上一根bar收盘价<下沿"这个门槛判断的始终是下沿本身（不是trigger_price），
  只负责确认"这是价格从下方第一次测试这个缺口"。
  一根bar内所有尚未失效的缺口都会分别检查，可能同一根bar触发多笔独立交易。

缺口生命周期（跟多头版镜像，上下沿互换）：
  实际触发开仓（价格突破下沿**且**MA当时确实在跌）后标记失效，不会重复开仓；
  如果价格突破下沿但MA当时不跌（信号被MA门槛挡掉），缺口**不**标记失效——
  留着等价格回落后，万一再次从下沿突破、且那时MA跌了，仍可以触发（允许
  重新测试同一缺口）。另外，不管MA状态如何，只要价格已经突破缺口**上沿**
  （完全回补整个缺口），一律立即判定失效、不再追踪——这一步必须独立于MA
  门槛，否则"MA不跌+价格已完全回补"的缺口会一直滞留在_active_fvgs里出不去。

止盈/止损（_resolve_exit/_bar_path）：
  enable_stop_loss=False时关闭止损，只剩止盈/收盘平仓两种结果（止损价仍按
  正常公式算出来、记进CSV的fvg_stop列，只是不参与结算——实现方式是把
  传给_resolve_exit的stop参数换成+inf，让"价格≥stop"这个判断恒不成立，
  不需要另外写一套没有止损分支的逻辑——跟多头版用-inf占位是同一个手法，
  方向相反）。
  目标=入场价-target_ticks×tick_size（空头靠下跌盈利）；止损=入场价+
  stop_loss_ticks×tick_size（固定价位止损，默认10个价位，止损价自然
  落在入场价之上；在_execute_short里按入场价当场算出来，不依赖缺口
  形态——跟多头版entry_price-stop_loss_ticks×tick_size是同一个手法，
  方向相反）。
  _bar_path这个"同一根bar内部价格运动路径"的假设跟多头版完全一样，不随
  交易方向变化（这是bar自身的K线形态决定的，跟某笔交易是做多还是做空无关）：
    阴线（close<open）：open → high → low → close
    阳线/平盘（close≥open）：open → low → high → close
  两个相邻锚点之间假定价格单调运动（不会越过两端更远）。_resolve_exit跟
  多头版的区别只在于方向镜像：先找到价格首次向上突破entry_price（缺口
  下沿）的位置（入场点，多头版是"向下跌破"），再从入场点开始，沿剩余路径
  逐段检查——每一段只可能单调朝一个方向运动，所以一段之内最多只能摸到
  target（下跌段，因为空头靠跌盈利）或stop（上涨段，空头止损在更高处）
  其中一个，不会两个都摸到；哪个先在路径顺序上出现就按哪个结算：
    先摸到target → 按目标价止盈离场（close_reason='target_hit'）
    先摸到stop   → 按止损价离场（close_reason='stop_hit'）
    都没摸到      → 按该bar收盘价离场（close_reason='bar_close'）
  入场发生的那一段如果是"突破entry_price后继续上涨"（比如阴线的open→high
  段、阳线的low→high段），这一段剩余部分（从入场点到段尾）只会越走越高，
  摸不到target，但**可能摸到stop**（止损价比入场价更高，同一段上涨足够猛
  时可以在入场后紧接着就被打中）——跟多头版"入场段继续下跌时可能先止损"
  是完全对称的情况。
  推导出的两个直观结论（跟多头版镜像，阳线/阴线互换）：
    阴线：入场发生在open→high段（因为阴线路径是open→high→low→close，
      向上突破entry_price最早只能发生在这一段），紧接着high→low段下跌，
      能不能摸到target等价于bar.close_price是否≤target（跟多头版"阴线
      看bar.close_price"对称，只是方向反过来）；入场那一段(open→high)
      本身如果冲到≥stop，则还没来得及回落就先止损了。
    阳线：入场发生在low→high段（发生在bar自己的low之后），入场后只剩
      high→close段，能不能摸到target等价于bar.close_price是否≤target；
      入场那一段(low→high)本身如果冲到≥stop，同样会先止损。
  跳空例外：如果open本身就已经高于entry_price，入场视为发生在bar最起点，
  整根bar都算入场之后，target/stop按各自方向正常检查。

记账方式：options_mode=True **只是为了借用stats.py里基于_closed_positions.
final_pnl的lifecycle记账路径**（跟ict_fvg_long完全相同的理由，详见该文件
文件头注释），不代表这是期权策略。final_pnl=entry_price-exit_price（空头，
价格下跌才赚钱，跟多头版exit_price-entry_price方向相反）。
"""
from qib_trader.core.models import BarData, Direction
from qib_trader.strategies.base import Strategy


class ict_fvg_short(Strategy):
    """ICT FVG 空头策略：两条MA同时下跌 + 价格反抽突破看跌缺口下沿 → 开空，
    当根bar止盈或收盘平仓，不跨bar持仓。"""

    options_mode:    bool  = True   # 见文件头注释：只为借用lifecycle记账路径
    initial_capital: float = 100_000.0

    ma_period:        int   = 5      # 第1条MA周期（收盘价），判断涨跌方向用
    ma2_period:       int   = 10     # 第2条MA周期，两条MA必须同时判定为跌势才放行，默认比ma_period更长
    enable_ma_filter: bool  = True   # False=关闭MA方向过滤（两条MA都不看），缺口+价位条件满足即可开仓
    tick_size:        float = 0.25   # 最小变动价位：NQ=0.25，股票通常=0.01，按品种配置
    entry_offset_ticks: float = 0.0  # 突破下沿后还要再涨这么多价位才真正开空（0=一突破下沿就开空）
    target_ticks:     float = 40.0   # 止盈价位数，entry-target_ticks×tick_size即止盈
    min_gap_ticks:    float = 20.0   # 缺口宽度(upper-lower)小于此值×tick_size则忽略，不追踪、不开仓
    enable_stop_loss: bool  = True    # False=关闭止损，只剩止盈/收盘平仓两种结果（止损价仍会算出来记进CSV，只是不参与结算）
    stop_loss_ticks:  float = 40.0   # 固定价位止损：亏此价位数就止损，止损价=入场价+stop_loss_ticks×tick_size

    def on_init(self) -> None:
        self._active_fvgs: list[dict] = []   # {'lower','upper','stop','formed_dt','used'}
        self._closed_positions: list[dict] = []
        self._position_seq = 0
        self._prev_close: float | None = None
        self._last_bar: BarData | None = None

    def on_start(self) -> None:
        self.active = True

    def on_stop(self) -> None:
        self.active = False
        # 每笔交易都在当根bar内结束生命周期，不存在跨bar持仓，无需force_close。

    # ── MA ──────────────────────────────────────────────────────────────────

    def _ma_at(self, bar_idx: int, period: int) -> float | None:
        """bar_idx位置的period周期收盘价均线，数据不足时返回None。"""
        if bar_idx < period - 1:
            return None
        window = self._own_bars[bar_idx - period + 1: bar_idx + 1]
        return sum(b.close_price for b in window) / period

    def _single_ma_falling(self, period: int) -> bool:
        """用_bar_idx-1/_bar_idx-2（不含当前这根信号bar）算指定周期均线的
        斜率，跟ict_fvg_long的_single_ma_rising是同一个修复（原来用当前bar
        自己的收盘价算MA，等于用了未来数据）。"""
        ma_now  = self._ma_at(self._bar_idx - 1, period)
        ma_prev = self._ma_at(self._bar_idx - 2, period)
        return ma_now is not None and ma_prev is not None and ma_now < ma_prev

    def _ma_falling(self) -> bool:
        """两条MA（ma_period/ma2_period）必须同时判定为跌势才放行，任一条
        不满足都算不通过。enable_ma_filter=False时直接返回True，相当于关掉
        整道门槛（两条MA都不看），只看缺口+价位条件。"""
        if not self.enable_ma_filter:
            return True
        return self._single_ma_falling(self.ma_period) and self._single_ma_falling(self.ma2_period)

    # ── FVG 识别与生命周期 ──────────────────────────────────────────────────

    def _detect_new_fvg(self, bar_idx: int) -> None:
        """用index-2/index-1/index这3根bar检测看跌型缺口（bar1.low>bar3.high）。
        中间那根bar（bar2）必须是阴线（close<open，跟多头版"必须阳线"镜像），
        要求造成这个缺口的位移bar本身也是下跌的，跟看跌缺口的方向一致，过滤
        "缺口形状对但中间bar其实在涨"这种杂音形态。止损改用固定价位（见
        stop_loss_ticks×tick_size），不依赖bar2的具体价位，只用它的K线颜色
        做这一道方向过滤。缺口宽度小于min_gap_ticks×tick_size时直接忽略，
        不加入追踪。在logic()里排在_check_entries之后调用，确保新缺口要到
        下一根bar才会被_check_entries检查到。"""
        if bar_idx < 2:
            return
        bar1 = self._own_bars[bar_idx - 2]
        bar2 = self._own_bars[bar_idx - 1]
        bar3 = self._own_bars[bar_idx]
        if bar1.low_price > bar3.high_price:
            if bar2.close_price >= bar2.open_price:
                return   # 中间bar是阳线/平盘，不是看跌型位移，不算有效缺口
            if bar1.low_price - bar3.high_price < self.min_gap_ticks * self.tick_size:
                return   # 缺口太窄，忽略
            self._active_fvgs.append({
                'lower': bar3.high_price, 'upper': bar1.low_price,
                'formed_dt': bar3.datetime, 'used': False,
            })

    def _check_entries(self, bar: BarData) -> None:
        """对每个尚未失效的缺口检查：价格是否从下往上突破其下沿、并再涨
        entry_offset_ticks个价位触及trigger_price；若此时两条MA都是跌的，
        则按trigger_price触发开空并标记失效。若触及trigger_price但MA当时
        不跌（任一条不跌都算），**不**标记失效——缺口留着，等价格回落后若再次从下沿突破、且
        那时MA跌了，仍可以触发（允许重新测试同一缺口）。另外，不管MA状态
        如何，只要价格已经突破
        上沿（缺口被完全回补），一律标记失效——这一步必须独立于MA门槛判断，
        跟ict_fvg_long里同一个bug修复的手法完全对称。
        一根bar内可能有多个缺口分别触发，互不影响。"""
        if self._prev_close is None or not self._active_fvgs:
            return
        ma_falling = self._ma_falling()
        for fvg in self._active_fvgs:
            if fvg['used']:
                continue
            lower = fvg['lower']
            trigger_price = lower + self.entry_offset_ticks * self.tick_size
            if self._prev_close < lower and bar.high_price >= trigger_price and ma_falling:
                self._execute_short(bar, trigger_price, fvg)
                fvg['used'] = True
            elif bar.high_price >= fvg['upper']:
                fvg['used'] = True   # 缺口已被完全回补，不管MA状态，都判定失效
        self._active_fvgs = [f for f in self._active_fvgs if not f['used']]

    # ── 开平仓（同一根bar内完成） ────────────────────────────────────────────

    def _bar_path(self, bar: BarData) -> list[float]:
        """阴线(close<open)按open→high→low→close；阳线/平盘(close≥open)按
        open→low→high→close。跟ict_fvg_long完全一样，这是bar自身的K线形态
        决定的路径假设，跟交易方向无关。"""
        if bar.close_price < bar.open_price:
            return [bar.open_price, bar.high_price, bar.low_price, bar.close_price]
        return [bar.open_price, bar.low_price, bar.high_price, bar.close_price]

    def _resolve_exit(self, bar: BarData, entry_price: float,
                      target: float, stop: float) -> tuple[float, str]:
        """沿_bar_path的顺序模拟：找到价格首次向上突破entry_price（入场）发生
        的位置，再从入场点开始沿剩余路径逐段检查。每一段单调朝一个方向运动，
        下跌段只可能摸到target（空头靠跌盈利），上涨段只可能摸到stop（空头
        止损在更高处），按路径顺序哪个先出现就按哪个结算。入场发生的那一段
        如果本身继续上涨，仍要检查是否摸到stop（跟target不同——target在这
        一段摸不到，但stop可能摸到）。跟ict_fvg_long的_resolve_exit是完全
        镜像的方向翻转（entry的穿越方向、target/stop各自对应的段方向全部
        反过来），_bar_path本身不变。"""
        points = self._bar_path(bar)
        entered = False
        for i in range(len(points) - 1):
            a, b = points[i], points[i + 1]
            if not entered:
                if a >= entry_price:
                    entered = True   # 跳空：这一段整体都在入场之后
                elif a < b and min(a, b) <= entry_price <= max(a, b):
                    entered = True   # 本段内部上穿entry_price，剩余部分继续上涨
                else:
                    continue
            if b < a and b <= target:
                return target, 'target_hit'
            if b > a and b >= stop:
                return stop, 'stop_hit'
        return bar.close_price, 'bar_close'

    def _execute_short(self, bar: BarData, entry_price: float, fvg: dict) -> None:
        target = entry_price - self.target_ticks * self.tick_size
        stop_price = entry_price + self.stop_loss_ticks * self.tick_size   # 固定价位止损，按入场价当场算出
        # enable_stop_loss=False时用+inf占位：b>=stop这个判断永远不成立，止损分支
        # 等于被禁用，回退到只有target_hit/bar_close两种结果；fvg_stop仍按正常
        # 公式算出来记进CSV，只是不参与结算。跟ict_fvg_long用-inf占位是同一个
        # 手法，方向相反。
        stop = stop_price if self.enable_stop_loss else float('inf')
        exit_price, close_reason = self._resolve_exit(bar, entry_price, target, stop)
        final_pnl = entry_price - exit_price   # 空头：价格下跌才赚钱

        self._position_seq += 1
        self._send_order(Direction.SHORT, entry_price, 1, "MKT")
        self._send_order(Direction.LONG, exit_price, 1, "MKT")
        self._closed_positions.append({
            'position_id':   self._position_seq,
            'entry_dt':      bar.datetime,
            'settlement_dt': bar.datetime,
            'entry_price':   entry_price,
            'exit_price':    exit_price,
            'fvg_lower':     fvg['lower'],
            'fvg_upper':     fvg['upper'],
            'fvg_stop':      stop_price,   # 始终记真实止损价，不管enable_stop_loss是否关闭
            'fvg_formed_dt': fvg['formed_dt'],
            'close_reason':  close_reason,
            'final_pnl':     final_pnl,
        })
        self.logger.info(
            f"[{bar.datetime}] 开空@{entry_price:.4f} 缺口=[{fvg['lower']:.4f},{fvg['upper']:.4f}] "
            f"止损@{stop_price:.4f}(启用={self.enable_stop_loss}) {close_reason}@{exit_price:.4f} pnl={final_pnl:.4f}"
        )

    # ── 主逻辑 ──────────────────────────────────────────────────────────────

    def logic(self, bar: BarData) -> None:
        self._check_entries(bar)          # 用本bar之前已存在的缺口（不含本bar刚形成的）
        self._detect_new_fvg(self._bar_idx)  # 本bar作为bar3，检测新缺口，下一根bar才会被用到
        self._record_mtm_pnl(bar)         # 恒为0：从不跨bar持仓
        self._prev_close = bar.close_price
        self._last_bar = bar
