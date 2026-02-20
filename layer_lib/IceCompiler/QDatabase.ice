#ifndef QDATABASE_ICE
#define QDATABASE_ICE

#include <QStruc.ice>

module IBTrader
{
	interface IQDatabase
	{
	    // 线程池
        int         IdlCount();             // 空闲线程数量
        bool        IsAllIdle();            // 是否全部空闲
        int         TaskCount();            // 积压的任务数量



		// 用tick更新数据库
		void		UpdateTickToDB(ITick tick);							// 把TICK更新到数据库各个表中

		// 查询最近更新情况
		ITick		GetLastUpdateTick(out int updateCount, out long recentUpdateSecnd);

		// 删除合约的数据（K线 指标）
		void		DelCodeId(string codeId, long beginTime,long endTime);

		// 原始TICK表
		void		RemoveAllTicks(string codeId);											// 删除某一品种的所有TICK表数据
		void		RemoveTicksByRange(string codeId,long beginTime,long endTime);			// 按时间区间删除某一品种的TICK表数据，区间为[beginTime,endTime)
		void		GetTicks(string codeId,IQuery query,out ITicks ticks);
		bool		GetOneTick(string codeId, long timePos, out ITick tick);				// 查询单个指定数据

		// KLine表
		void		UpdateKLine(string codeId, ITimeType timeType, IKLine kline);
		void		UpdateKLines(string codeId, ITimeType timeType, IKLines klines);
		void		RemoveAllKLines(string codeId, ITimeType timeType);										// 删除某一品种的所有kline表数据
		void		RemoveKLinesByRange(string codeId, ITimeType timeType, long beginTime, long endTime);	// 按时间区间删除某一品种kline表数据，区间为[beginTime,endTime)
		void		RemoveOneKLine(string codeId, ITimeType timeType, long timePos);						// 删除某一品种一条kline记录
		void		GetKLines(string codeId, ITimeType timeType, IQuery query,out IKLines klines);
		bool		GetOneKLine(string codeId, ITimeType timeType, long timePos, out IKLine kline);			// 查询单个指定数据
		void		GetKLinePairs(string first, string second, ITimeType timeType, IQuery query,out IKLinePairs kLinePairs);
		void		GetInvalidKLines(string codeId, ITimeType timeType, out IKLines klines);				// 得到非法的BAR为清理数据库做准备

		// 指标更新
		void		RecountAllIndex(string codeId, ITimeType timeType);										// 全量计算某品种所有指标
		void		UpdateAllIndexFromTimePos(string codeId, ITimeType timeType, long timePos);			    // 增量计算某品种指定时间点(毫秒)之后的所有指标

		// Ma表
		void		GetMas(string codeId, ITimeType timeType, IQuery query,out IAvgValues mas);
		bool		GetOneMa(string codeId, ITimeType timeType, long timePos, out IAvgValue ma);			// 查询单个指定数据

		// VwMa表
		void		GetVwMas(string codeId, ITimeType timeType, IQuery query,out IAvgValues mas);
		bool		GetOneVwMa(string codeId, ITimeType timeType, long timePos, out IAvgValue ma);			// 查询单个指定数据

		// Ema表
		void		GetEmas(string codeId, ITimeType timeType, IQuery query,out IAvgValues emas);
		bool		GetOneEma(string codeId, ITimeType timeType, long timePos, out IAvgValue ema);			// 查询单个指定数据

		// macd表
		void		GetMacds(string codeId, ITimeType timeType, IQuery query,out IMacdValues macds);
		bool		GetOneMacd(string codeId, ITimeType timeType, long timePos, out IMacdValue ma);			// 查询单个指定数据

		// divType表
		void		GetDivTypes(string codeId, ITimeType timeType, IQuery query,out IDivTypeValues divtypes);
		bool		GetOneDivType(string codeId, ITimeType timeType, long timePos, out IDivTypeValue divtype);	// 查询单个指定数据

		// atr表
		void		GetAtrs(string codeId, ITimeType timeType, IQuery query,out IAtrValues avgAtrs);
		bool		GetOneAtr(string codeId, ITimeType timeType, long timePos, out IAtrValue avgAtr);	    // 查询单个指定数据

	};
};

#endif
