import threading
import Ice
import IBTrader
import pandas as pd
from datetime import datetime
from qib_trader.utils.tool import TimeUtils
from qib_trader.utils.config_loader import get_env_config


class FactoryIce:

    _proxy = None
    _communicator = None
    _lock = threading.Lock()

    def __init__(self):
        raise RuntimeError("FactoryIce 单例禁止实例化")

    @classmethod
    def make_and_get_proxy(cls):
        if cls._proxy is None:
            with cls._lock:
                if cls._proxy is None:
                    cls._create_proxy()

        return cls._proxy

    @classmethod
    def _create_proxy(cls):
        proxy_str = get_env_config().get("qdatabase_proxy", "QServer/IBQDatabase:tcp -h 10.0.0.64 -p 10000")
        cls._communicator = Ice.initialize()
        base = cls._communicator.stringToProxy(proxy_str)
        cls._proxy = IBTrader.IQDatabasePrx.checkedCast(base)



    @classmethod
    def shutdown(cls):
        if cls._communicator:
            cls._communicator.destroy()
            cls._communicator = None

    @classmethod
    def make_timepair(cls, beginPos : str, endPos : str):
        """
        用字符串制作IBTrader.ITimeType
        """
        time_pair = IBTrader.ITimePair()
        time_pair.beginPos = TimeUtils.str_to_ms(beginPos)
        time_pair.endPos = TimeUtils.str_to_ms(endPos)
        return time_pair

    # @classmethod
    # def get_klines_lastday(cls, code_id : str, timestr: str):
    #     db_proxy = cls.make_and_get_proxy();
    #     success, kline = db_proxy.GetLastDayKLine(code_id, TimeUtils.str_to_ms(timestr))
    #     print(f"success = {success}, data = {kline}")
    #     print(TimeUtils.ms_to_str(kline.time))
    #     return

    @classmethod
    def get_klines_loop(cls, code_id : str, time_type : IBTrader.ITimeType, time_pair : IBTrader.ITimePair):
        """
        循环取数据[beginPos, endPos)
        """

        db_proxy = cls.make_and_get_proxy();

        query = IBTrader.IQuery()
        query.byReqType = 3 # 3表示请求某个时间以后多少个单位的数据(dwSubscribeNum为0时表示该时间后所有的数据)
        query.dwSubscribeNum = 4000

        # 调用 ICE 接口 (返回 List[IKLine])
        klines: list[IBTrader.IKLine] = []
        while True:
            if len(klines) == 0:
                query.tTime = time_pair.beginPos
            else:
                query.tTime = klines[-1].time + 1 # 加1毫秒避免重复查询

            # 开始查询
            tem_klines: list[IBTrader.IKLine] = db_proxy.GetKLines(code_id, time_type, query)
            if len(tem_klines) == 0:
                break

            klines.extend(kline for kline in tem_klines if kline.time < time_pair.endPos)
            if tem_klines[-1].time >= time_pair.endPos:
                break


            continue
        return klines

    # ── Enriched klines: GetRichs 一次返回 kline + MACD + DivType + ATR + 上日高低 ──

    @classmethod
    def _generic_loop(cls, getter_name: str, code_id: str, time_type, time_pair):
        """通用分页循环查询，适用于所有具备 .time 字段的接口。"""
        db_proxy = cls.make_and_get_proxy()
        getter = getattr(db_proxy, getter_name)
        query = IBTrader.IQuery()
        query.byReqType = 3
        query.dwSubscribeNum = 4000
        results = []
        while True:
            query.tTime = results[-1].time + 1 if results else time_pair.beginPos
            tem = getter(code_id, time_type, query)
            if not tem:
                break
            results.extend(r for r in tem if r.time < time_pair.endPos)
            if tem[-1].time >= time_pair.endPos:
                break
        return results

    @classmethod
    def _divtype_str(cls, dt_enum) -> str:
        if dt_enum == IBTrader.IDivType.BOTTOM:    return "Bottom"
        if dt_enum == IBTrader.IDivType.BOTTOMSUB: return "BottomSub"
        if dt_enum == IBTrader.IDivType.TOP:       return "Top"
        if dt_enum == IBTrader.IDivType.TOPSUB:    return "TopSub"
        return "Normal"

    @classmethod
    def _richs_to_df(cls, richs) -> pd.DataFrame:
        """IRichValues → DataFrame，列名与 enriched_df_to_bars 匹配。"""
        if not richs:
            return pd.DataFrame()
        data = [{
            'time':          r.time,
            'datetime':      datetime.fromtimestamp(r.time / 1000.0),
            'open_price':    float(r.open),
            'high_price':    float(r.high),
            'low_price':     float(r.low),
            'close_price':   float(r.close),
            'volume':        int(r.vol),
            'dif':           float(r.dif),
            'dea':           float(r.dea),
            'macd':          float(r.macd),
            'div_type':      cls._divtype_str(r.divType),
            'is_uturn':      bool(r.isUTurn),
            'atr':           float(r.thisAtr),
            'avg_atr':       float(r.avgAtr),
            'prev_day_high':  float(r.preDayHigh),
            'prev_day_low':   float(r.preDayLow),
            'prev_day_close': float(r.preDayClose),
        } for r in richs]
        df = pd.DataFrame(data)
        df.set_index('datetime', inplace=True, drop=False)
        return df

    @classmethod
    def get_enriched_klines_loop(cls, code_id: str, time_type, time_pair) -> pd.DataFrame:
        """用 GetRichs 一次拉取所有 enriched 数据，返回 DataFrame。"""
        richs = cls._generic_loop("GetRichs", code_id, time_type, time_pair)
        return cls._richs_to_df(richs)


if __name__ == "__main__":

    # ("NQ", "2025-01-16 12:00:00")
    

    mylist = [1,2,3,4,5,6,7,8,9]
    newlist = mylist[:4]
    print(newlist)

    klines = FactoryIce.get_klines_loop(
        code_id = "NQ",
        time_type= IBTrader.ITimeType.M15,
        time_pair = FactoryIce.make_timepair("2025-01-01 00:00:00", "2026-01-01 00:00:00")
    )
    print(f"klines begin: {TimeUtils.ms_to_str(klines[0].time)}")
    print(f"klines last: {TimeUtils.ms_to_str(klines[-1].time)}")

    FactoryIce.shutdown()

