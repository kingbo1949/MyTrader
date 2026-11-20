"""
实盘信号系统上线前的只读连接验证脚本。全程不下单——只用 IBGateway 暴露的只读
方法（见 live/ib_gateway.py 头部的安全边界说明）。

对应设计计划"验证"一节里还没跑过的两项：
  1. 账户的期权实时/延迟行情权限档位（决定 GUI"参考真实bid/ask"这一列是否能显示）。
  2. `ib.fillEvent` 确实能收到真实成交推送（`Reconciler` 思路A对照功能的数据源，
     必须在正式跑 live_runner.py 之前确认能跑通——可以在监听期间去 TWS 里手动
     下一笔试单验证）。
连接/账户读取/当前期权链这几项已经用真实账户手动验证过（结论记在实盘系统设计
文档里），这个脚本不重复做。

用法：
  python3 scripts/verify_ib_connection.py                       # 用 live.yaml 里第一个启用策略的 code_id
  python3 scripts/verify_ib_connection.py --code-id SMH
  python3 scripts/verify_ib_connection.py --listen-seconds 120   # 拉长监听时间，方便去TWS手动试单
  python3 scripts/verify_ib_connection.py --skip-fills           # 只测行情权限，不监听成交
"""
import argparse
import logging
import sys
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from qib_trader.live.ib_gateway import IBGateway
from qib_trader.live.option_match import days_to
from qib_trader.utils.config_loader import get_live_ib_config, get_live_strategies_config
from qib_trader.utils.log_setup import setup_logging

logger = logging.getLogger("VerifyIBConnection")


def _default_code_id() -> str:
    for s_config in get_live_strategies_config():
        if s_config.get("enable", False):
            return s_config["code_id"]
    return "SMH"


def _check_account(gateway: IBGateway) -> None:
    accounts = gateway.managed_accounts()
    logger.info(f"managedAccounts: {accounts}")
    for row in gateway.account_summary():
        if row.tag in ("NetLiquidation", "AvailableFunds", "BuyingPower"):
            logger.info(f"  {row.tag}: {row.value} {row.currency}")


def _check_market_data_permission(gateway: IBGateway, code_id: str) -> None:
    """查一个近月ATM合约的真实bid/ask，确定账户的行情权限档位（实时/延迟/无）。"""
    chain = gateway.get_option_chain(code_id)
    if not chain.expirations or not chain.strikes:
        logger.warning(f"{code_id} 期权链为空（reqSecDefOptParams 无返回），跳过行情权限检查")
        return

    underlying = gateway.qualify_underlying(code_id)
    gateway.ib.reqMarketDataType(1)
    ticker = gateway.ib.reqTickers(underlying)[0]
    S = ticker.marketPrice()
    if not S or S != S:   # NaN 或 0：没有实时权限，退回延迟数据
        gateway.ib.reqMarketDataType(3)
        ticker = gateway.ib.reqTickers(underlying)[0]
        S = ticker.marketPrice()
    if not S or S != S:
        logger.warning(f"{code_id} 无法取得标的现价（实时/延迟都试过），跳过期权行情权限检查")
        return

    now = datetime.now()
    near_expiry = min(chain.expirations, key=lambda e: abs(days_to(e, now) - 14))
    atm_strike = min(chain.strikes, key=lambda k: abs(k - S))
    quote = gateway.get_reference_quote(code_id, near_expiry, atm_strike, 'call')
    if quote:
        logger.info(f"期权行情权限正常: {code_id} {near_expiry} {atm_strike}C "
                   f"bid/ask={quote['bid']}/{quote['ask']} mid={quote['mid']}")
    else:
        logger.warning(f"{code_id} {near_expiry} {atm_strike}C 拿不到任何bid/ask（实时/延迟都试过），"
                       f"账户可能没有该品种的期权行情权限——GUI里的\"参考真实报价\"这一列会一直是空的，"
                       f"不影响真实链匹配（到期日/行权价）本身的展示")


def _listen_for_fills(gateway: IBGateway, duration_s: float) -> list:
    received = []

    def on_fill(trade, fill):
        contract = trade.contract
        execution = fill.execution
        logger.info(f"收到成交推送: {contract.symbol} "
                   f"{getattr(contract, 'lastTradeDateOrContractMonth', '')} "
                   f"{getattr(contract, 'strike', '')}{getattr(contract, 'right', '')} "
                   f"price={execution.price} shares={execution.shares} "
                   f"side={execution.side} time={execution.time}")
        received.append((trade, fill))

    gateway.subscribe_fills(on_fill)
    logger.info(f"开始监听 fillEvent，持续 {duration_s:.0f} 秒——如需验证事件确实能收到，"
               f"请在此期间到 TWS 里手动下一笔试单并成交")
    gateway.ib.sleep(duration_s)
    logger.info(f"监听结束，共收到 {len(received)} 笔成交推送")

    todays_execs = gateway.ib.reqExecutions()
    logger.info(f"reqExecutions() 补查（当日全部成交，只读）：共 {len(todays_execs)} 笔")
    return received


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--code-id", default=None, help="用于测试期权链/行情权限的品种，默认取live.yaml里第一个启用策略的code_id")
    parser.add_argument("--listen-seconds", type=float, default=30.0, help="监听fillEvent的秒数")
    parser.add_argument("--skip-fills", action="store_true", help="跳过fillEvent监听，只测账户信息+行情权限")
    args = parser.parse_args()

    setup_logging()
    ib_config = get_live_ib_config()
    code_id = args.code_id or _default_code_id()

    gateway = IBGateway(ib_config['host'], ib_config['port'], ib_config.get('client_id', 17))
    logger.info(f"连接 {ib_config['host']}:{ib_config['port']} clientId={ib_config.get('client_id', 17)} "
               f"—— 请确认这是预期的账户（可能是真实资金账户）")
    gateway.connect(timeout=ib_config.get('timeout_seconds', 15))
    try:
        _check_account(gateway)
        _check_market_data_permission(gateway, code_id)
        if not args.skip_fills:
            _listen_for_fills(gateway, args.listen_seconds)
    finally:
        gateway.disconnect()
        logger.info("已断开连接")


if __name__ == "__main__":
    main()
