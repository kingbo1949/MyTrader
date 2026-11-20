"""
ib_insync 只读连接封装。

安全边界（结构性，不只是靠约定）：本文件只 import `IB`/`Stock`/`Future`/`Option`，
**不 import `Order`/`Trade`**——这个阶段的代码路径不可能构造出下单请求。

允许调用：connect/disconnect/qualifyContracts/reqContractDetails/reqSecDefOptParams/
reqTickers/reqMktData(snapshot)/reqMarketDataType/reqManagedAccts/reqAccountSummary/
positions/accountValues/fillEvent/execDetailsEvent/reqExecutions。
禁止：placeOrder/cancelOrder/whatIfOrder/任何 Order/Trade 对象构造——本文件里没有,
以后维护时也不要加。
"""
import logging

from ib_insync import IB, Stock, Future, Option

from qib_trader.live.option_match import OptionChainMeta
from qib_trader.utils import option_pricing

logger = logging.getLogger("IBGateway")


class IBGateway:

    def __init__(self, host: str, port: int, client_id: int):
        self._host, self._port, self._client_id = host, port, client_id
        self.ib = IB()
        self._underlying_cache: dict = {}
        self._chain_cache: dict = {}

    # ── 连接 ──────────────────────────────────────────────────────────────

    def connect(self, timeout: float = 15.0) -> None:
        self.ib.connect(self._host, self._port, clientId=self._client_id, timeout=timeout)
        logger.info(f"已连接 {self._host}:{self._port} clientId={self._client_id} "
                    f"server_version={self.ib.client.serverVersion()}")

    def disconnect(self) -> None:
        if self.ib.isConnected():
            self.ib.disconnect()

    @property
    def connected(self) -> bool:
        return self.ib.isConnected()

    # ── 账户信息（只读，供 verify 脚本 / GUI 状态栏用）────────────────────────

    def managed_accounts(self) -> list:
        return self.ib.managedAccounts()

    def account_summary(self, account: str = '') -> list:
        return self.ib.accountSummary(account)

    def positions(self) -> list:
        return self.ib.positions()

    # ── 合约解析 ──────────────────────────────────────────────────────────

    def qualify_underlying(self, code_id: str):
        """目前只支持股票/ETF（SMH/QQQ 等）。期货标的（ES/NQ）的具体到期合约月份解析
        更复杂，v1 的 live.yaml 只配了 SMH+QQQ，先不实现，真要接期货策略时再补。"""
        if code_id in self._underlying_cache:
            return self._underlying_cache[code_id]
        if option_pricing.is_futures(code_id):
            raise NotImplementedError(
                f"{code_id} 是期货标的，qualify_underlying 目前只支持股票/ETF，"
                f"接期货策略前需要补充具体到期合约的解析逻辑")
        contract = Stock(code_id, 'SMART', 'USD')
        qualified = self.ib.qualifyContracts(contract)
        if not qualified:
            raise RuntimeError(f"无法解析标的合约: {code_id}")
        self._underlying_cache[code_id] = qualified[0]
        return qualified[0]

    def get_option_chain(self, code_id: str) -> OptionChainMeta:
        """只读元数据查询（到期日/行权价列表），不涉及任何行情权限。"""
        if code_id in self._chain_cache:
            return self._chain_cache[code_id]
        underlying = self.qualify_underlying(code_id)
        chains = self.ib.reqSecDefOptParams(
            underlying.symbol, '', underlying.secType, underlying.conId)
        chain = next((c for c in chains if c.exchange == 'SMART'), chains[0] if chains else None)
        if chain is None:
            meta = OptionChainMeta(expirations=[], strikes=[])
            logger.warning(f"{code_id} 未获取到期权链")
        else:
            meta = OptionChainMeta(expirations=sorted(chain.expirations),
                                   strikes=sorted(chain.strikes))
        self._chain_cache[code_id] = meta
        return meta

    # ── 参考真实报价（锦上添花，非阻塞，拿不到就返回 None）───────────────────

    def get_reference_quote(self, code_id: str, expiry: str, strike: float, right: str) -> dict | None:
        try:
            opt = Option(code_id, expiry, strike, right[0].upper(), 'SMART', currency='USD')
            qualified = self.ib.qualifyContracts(opt)
            if not qualified:
                return None
            contract = qualified[0]
            self.ib.reqMarketDataType(1)   # 优先实时
            ticker = self.ib.reqTickers(contract)[0]
            bid, ask = ticker.bid, ticker.ask
            if not (bid and ask and bid > 0 and ask > 0):
                self.ib.reqMarketDataType(3)   # 退回延迟数据
                ticker = self.ib.reqTickers(contract)[0]
                bid, ask = ticker.bid, ticker.ask
            if bid and ask and bid > 0 and ask > 0:
                return {'bid': bid, 'ask': ask, 'mid': round((bid + ask) / 2, 4)}
        except Exception as exc:
            logger.debug(f"参考报价查询失败 {code_id} {expiry} {strike} {right}: {exc}")
        return None

    # ── 真实成交事件订阅（只读推送，供 reconciliation.py 用）─────────────────

    def subscribe_fills(self, callback) -> None:
        """callback(trade, fill) -> None，签名跟 ib_insync 的 fillEvent 一致。"""
        self.ib.fillEvent += callback
