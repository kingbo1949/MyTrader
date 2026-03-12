import threading
import Ice
import IBTrader
from datetime import datetime
from tool import TimeUtils
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


if __name__ == "__main__":
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

