import bisect
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

    @classmethod
    def get_klines_lastday(cls, code_id : str, timestr: str):
        db_proxy = cls.make_and_get_proxy();
        success, kline = db_proxy.GetLastDayKLine(code_id, TimeUtils.str_to_ms(timestr))
        print(f"success = {success}, data = {kline}")
        print(TimeUtils.ms_to_str(kline.time))
        return

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

    # ── Enriched klines: klines + MACD + DivType + ATR + prev-day D1 ──────────

    @classmethod
    def _generic_loop(cls, getter_name: str, code_id: str, time_type, time_pair):
        """通用分页循环查询，适用于所有具备 .time 字段的指标接口。"""
        db_proxy = cls.make_and_get_proxy()
        getter = getattr(db_proxy, getter_name)
        query = IBTrader.IQuery()
        query.byReqType = 3 # 3表示请求某个时间以后多少个单位的数据(dwSubscribeNum为0时表示该时间后所有的数据)
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
    def _get_macds_loop(cls, code_id, time_type, time_pair):
        return cls._generic_loop("GetMacds", code_id, time_type, time_pair)

    @classmethod
    def _get_divtypes_loop(cls, code_id, time_type, time_pair):
        return cls._generic_loop("GetDivTypes", code_id, time_type, time_pair)

    @classmethod
    def _get_atrs_loop(cls, code_id, time_type, time_pair):
        return cls._generic_loop("GetAtrs", code_id, time_type, time_pair)

    @classmethod
    def _divtype_str(cls, dt_enum) -> str:
        if dt_enum == IBTrader.IDivType.BOTTOM:    return "Bottom"
        if dt_enum == IBTrader.IDivType.BOTTOMSUB: return "BottomSub"
        if dt_enum == IBTrader.IDivType.TOP:       return "Top"
        if dt_enum == IBTrader.IDivType.TOPSUB:    return "TopSub"
        return "Normal"

    @classmethod
    def _macd_df(cls, macds):
        if not macds:
            return None
        return pd.DataFrame([{'time': m.time, 'dif': m.dif, 'dea': m.dea} for m in macds])

    @classmethod
    def _divtype_df(cls, divtypes):
        if not divtypes:
            return None
        return pd.DataFrame([{
            'time': d.time,
            'div_type': cls._divtype_str(d.divType),
            'is_uturn': bool(d.isUTurn)
        } for d in divtypes])

    @classmethod
    def _atr_df(cls, atrs):
        if not atrs:
            return None
        return pd.DataFrame([{'time': a.time, 'atr': float(a.thisAtr)} for a in atrs])

    @classmethod
    def _merge_indicators(cls, klines_df, macds, divtypes, atrs) -> pd.DataFrame:
        """Left-join MACD / DivType / ATR onto klines DataFrame (keyed on 'time')."""
        df = klines_df.copy().reset_index(drop=True)
        for indicator_df, defaults in [
            (cls._macd_df(macds),     {'dif': 0.0, 'dea': 0.0}),
            (cls._divtype_df(divtypes), {'div_type': '', 'is_uturn': False}),
            (cls._atr_df(atrs),        {'atr': 0.0}),
        ]:
            if indicator_df is not None:
                df = df.merge(indicator_df, on='time', how='left')
            for col, val in defaults.items():
                if col not in df.columns:
                    df[col] = val
                else:
                    df[col] = df[col].fillna(val)
        return df

    @classmethod
    def _build_d1_map(cls, d1_klines):
        """Build {date: (high, low, close)} dict and sorted date list."""
        d1_dict = {}
        for k in d1_klines:
            dt = datetime.fromtimestamp(k.time / 1000)
            d1_dict[dt.date()] = (float(k.high), float(k.low), float(k.close))
        return d1_dict, sorted(d1_dict.keys())

    @classmethod
    def _prev_day_hlc(cls, m15_date, sorted_dates, d1_dict):
        """Return prev trading day (high, low, close) for a given date."""
        idx = bisect.bisect_left(sorted_dates, m15_date)
        if idx == 0:
            return 0.0, 0.0, 0.0
        return d1_dict[sorted_dates[idx - 1]]

    @classmethod
    def _merge_prev_day(cls, m15_df, d1_klines) -> pd.DataFrame:
        """Attach prev_day_high / prev_day_low / prev_day_close to each M15 row."""
        df = m15_df.copy()
        if not d1_klines:
            df['prev_day_high'] = df['prev_day_low'] = df['prev_day_close'] = 0.0
            return df
        d1_dict, sorted_dates = cls._build_d1_map(d1_klines)
        rows = [cls._prev_day_hlc(r.datetime.date(), sorted_dates, d1_dict)
                for r in df.itertuples(index=False)]
        df['prev_day_high']  = [r[0] for r in rows]
        df['prev_day_low']   = [r[1] for r in rows]
        df['prev_day_close'] = [r[2] for r in rows]
        return df

    @classmethod
    def _make_d1_time_pair(cls, m15_time_pair):
        """D1 range: 7 days before M15 start through M15 end."""
        d1_pair = IBTrader.ITimePair()
        d1_pair.beginPos = m15_time_pair.beginPos - 7 * 24 * 3600 * 1000
        d1_pair.endPos   = m15_time_pair.endPos
        return d1_pair

    @classmethod
    def get_enriched_klines_loop(cls, code_id: str, time_type, time_pair) -> pd.DataFrame:
        """拉取 M15 klines 并合并 MACD / DivType / ATR / prev-day D1，返回 DataFrame。"""
        from qib_trader.api.converters import IceConverter
        klines = cls.get_klines_loop(code_id, time_type, time_pair)
        if not klines:
            return pd.DataFrame()
        macds    = cls._get_macds_loop(code_id, time_type, time_pair)
        divtypes = cls._get_divtypes_loop(code_id, time_type, time_pair)
        atrs     = cls._get_atrs_loop(code_id, time_type, time_pair)
        d1_klines = cls.get_klines_loop(
            code_id, IBTrader.ITimeType.D1, cls._make_d1_time_pair(time_pair)
        )
        klines_df = IceConverter.klines_to_df(klines)
        df = cls._merge_indicators(klines_df, macds, divtypes, atrs)
        return cls._merge_prev_day(df, d1_klines)


if __name__ == "__main__":

    FactoryIce.get_klines_lastday("NQ", "2025-01-16 12:00:00")
    

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

