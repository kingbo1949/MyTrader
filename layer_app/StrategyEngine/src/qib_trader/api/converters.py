import pandas as pd
import numpy as np
from typing import List, Any, Optional
from datetime import datetime

# 导入根据 QStruc.py 生成的 Python 模块
import IBTrader

# [修改] 导入核心数据模型，增加 Interval 枚举
from qib_trader.core.models import BarData, Interval, CodeId, BarDatas

class IceConverter:
    """
    针对 IBTrader IQDatabase 接口的高性能转换器。

    职责：
    1. 将 Ice List[Struct] 转换为向量化容器 (Pandas/NumPy)。
    2. 将向量化容器还原为策略层使用的 Dataclass 对象。
    """

    @staticmethod
    def klines_to_df(klines: List[IBTrader.IKLine]) -> pd.DataFrame:
        """
        将 Ice 返回的 IKLines 列表转换为高性能 Pandas DataFrame。
        """
        if not klines:
            return pd.DataFrame()

        data = [
            {"time": k.time, "open": k.open, "high": k.high,
             "low": k.low, "close": k.close, "vol": k.vol}
            for k in klines
        ]
        df = pd.DataFrame(data)

        # 时间戳转换：将 long 映射为 datetime64[ns]
        if 'time' in df.columns:
            df['datetime'] = pd.to_datetime(
                df['time'].apply(lambda ms: datetime.fromtimestamp(ms / 1000.0))
            )
            # 保持 drop=False 以便 itertuples 访问 datetime 字段
            df.set_index('datetime', inplace=True, drop=False)

        # [关键重构] 字段重命名：对齐 QStruc.py 与系统内部模型
        column_mapping = {
            'open': 'open_price',
            'high': 'high_price',
            'low': 'low_price',
            'close': 'close_price',
            'vol': 'volume'
        }
        df.rename(columns=column_mapping, inplace=True)

        return df

    @staticmethod
    def df_to_bars(df: pd.DataFrame, codeId: CodeId, interval_str: str) -> BarDatas:
        """
        将 DataFrame 转化为 Python 3.10 dataclass 列表。
        [修改] 增加了 interval_str 参数，并将其转换为 Interval 枚举注入 BarData
        """
        if df.empty:
            return []

        # [新增] 将字符串转换为强类型枚举，增加系统健壮性
        try:
            interval_enum = Interval(interval_str)
        except ValueError:
            raise ValueError(f"不支持的周期字符串: {interval_str}，请检查 models.Interval 定义")

        # [保持高性能] 利用 itertuples 配合 slots=True 提高生成效率
        return [
            BarData(
                codeId=codeId,
                datetime=row.datetime,
                interval=interval_enum,  # [新增] 注入周期信息
                open_price=float(row.open_price),
                high_price=float(row.high_price),
                low_price=float(row.low_price),
                close_price=float(row.close_price),
                volume=int(row.volume)
            )
            for row in df.itertuples(index=False)
        ]

    @staticmethod
    def klines_to_numpy(klines: List[IBTrader.IKLine]) -> np.ndarray:
        """
        极致性能：直接转换为 NumPy 结构化数组，适合大规模指标计算。
        [保持不变] 仅作为高性能备选路径
        """
        if not klines:
            return np.array([])

        dtype = [
            ('time', 'i8'),
            ('close', 'f8'),
            ('open', 'f8'),
            ('high', 'f8'),
            ('low', 'f8'),
            ('vol', 'i8')
        ]

        return np.fromiter(
            ((k.time, k.close, k.open, k.high, k.low, k.vol) for k in klines),
            dtype=dtype
        )

    @staticmethod
    def macds_to_df(macds: List[IBTrader.IMacdValue]) -> pd.DataFrame:
        """
        转换指标数据：IMacdValues -> DataFrame。
        [保持不变] 对应 QStruc.py 中的字段：dif, dea, macd
        """
        if not macds:
            return pd.DataFrame()

        data = [m.__dict__ for m in macds]
        df = pd.DataFrame(data)

        if 'time' in df.columns:
            df['datetime'] = pd.to_datetime(df['time'], unit='ms')
            df.set_index('datetime', inplace=True)

        return df[['dif', 'dea', 'macd']]

    @staticmethod
    def enriched_df_to_bars(df: pd.DataFrame, codeId: CodeId, interval_str: str) -> BarDatas:
        """
        将 get_enriched_klines_loop 返回的 DataFrame 转换为 BarData 列表。
        用 getattr 安全读取扩展字段，确保向后兼容。
        """
        if df.empty:
            return []
        try:
            interval_enum = Interval(interval_str)
        except ValueError:
            raise ValueError(f"不支持的周期字符串: {interval_str}")
        return [
            BarData(
                codeId=codeId,
                datetime=row.datetime,
                interval=interval_enum,
                open_price=float(row.open_price),
                high_price=float(row.high_price),
                low_price=float(row.low_price),
                close_price=float(row.close_price),
                volume=int(row.volume),
                dif           =float(getattr(row, 'dif',            0.0)),
                dea           =float(getattr(row, 'dea',            0.0)),
                div_type      =str  (getattr(row, 'div_type',       '')),
                is_uturn      =bool (getattr(row, 'is_uturn',       False)),
                atr           =float(getattr(row, 'atr',            0.0)),
                prev_day_high =float(getattr(row, 'prev_day_high',  0.0)),
                prev_day_low  =float(getattr(row, 'prev_day_low',   0.0)),
                prev_day_close=float(getattr(row, 'prev_day_close', 0.0)),
            )
            for row in df.itertuples(index=False)
        ]