#ifndef QDATABASE_ICE
#define QDATABASE_ICE

#include <QStruc.ice>

module IBTrader
{
	interface IQDatabase
	{
		// 用tick更新数据库
		void		UpdateTickToDB(ITick tick);							// 把TICK更新到数据库各个表中

		// 查询最近更新情况
		ITick		GetLastUpdateTick(out int updateCount, out long recentUpdateSecnd);


		// 合约表部分
        // void		AddContract(IContract contract);					// 更新或添加合约 
		// bool		GetContract(string codeId,out IContract contract);	// 查询合约
		// void		GetAllContracts(out IContracts contracts);			// 查询合约
		

		// 原始TICK表
		void		RemoveAllTicks(string codeId);											// 删除某一品种的所有TICK表数据
		void		RemoveTicksByRange(string codeId,long beginTime,long endTime);			// 按时间区间删除某一品种的TICK表数据，区间为[beginTime,endTime)
		void		GetTicks(string codeId,IQuery query,out ITicks ticks);
		bool		GetOneTick(string codeId, long timePos, out ITick tick);				// 查询单个指定数据

		// KLine表
		void		UpdateKLine(string codeId, ITimeType timeType, IKLine kline);
		void		RemoveAllKLines(string codeId, ITimeType timeType);										// 删除某一品种的所有kline表数据
		void		RemoveKLinesByRange(string codeId, ITimeType timeType, long beginTime, long endTime);	// 按时间区间删除某一品种kline表数据，区间为[beginTime,endTime)
		void		RemoveOneKLine(string codeId, ITimeType timeType, long timePos);						// 删除某一品种一条kline记录
		void		GetKLines(string codeId, ITimeType timeType, IQuery query,out IKLines klines);
		bool		GetOneKLine(string codeId, ITimeType timeType, long timePos, out IKLine kline);			// 查询单个指定数据
		void		GetKLinePairs(string first, string second, ITimeType timeType, IQuery query,out IKLinePairs kLinePairs);
		void		GetInvalidKLines(string codeId, ITimeType timeType, out IKLines klines);				// 得到非法的BAR为清理数据库做准备

		// Ma表
		void		RecountMa(string codeId, ITimeType timeType);											// 重新计算某品种所有MA, 会删除表全部重新算
		void		RecountMaFromTimePos(string codeId, ITimeType timeType, long timePos);					// 重新计算某品种指定时间点(毫秒)之后的所有MA                               
		void		UpdateMa(string codeId, ITimeType timeType, IAvgValue ma);
		void		RemoveAllMas(string codeId, ITimeType timeType);										// 删除某一品种的所有ma表数据
		void		RemoveMasByRange(string codeId, ITimeType timeType, long beginTime, long endTime);		// 按时间区间删除某一品种的ma表数据，区间为[beginTime,endTime)
		void		GetMas(string codeId, ITimeType timeType, IQuery query,out IAvgValues mas);
		bool		GetOneMa(string codeId, ITimeType timeType, long timePos, out IAvgValue ma);			// 查询单个指定数据

		// VwMa表
		void		RecountVwMa(string codeId, ITimeType timeType);											// 重新计算某品种所有VwMA, 会删除表全部重新算
		void		RecountVwMaFromTimePos(string codeId, ITimeType timeType, long timePos);				// 重新计算某品种指定时间点(毫秒)之后的所有VwMA                         
		void		UpdateVwMa(string codeId, ITimeType timeType, IAvgValue ma);
		void		RemoveAllVwMas(string codeId, ITimeType timeType);										// 删除某一品种的所有VwMa表数据
		void		RemoveVwMasByRange(string codeId, ITimeType timeType, long beginTime, long endTime);	// 按时间区间删除某一品种的VwMa表数据，区间为[beginTime,endTime)
		void		GetVwMas(string codeId, ITimeType timeType, IQuery query,out IAvgValues mas);
		bool		GetOneVwMa(string codeId, ITimeType timeType, long timePos, out IAvgValue ma);			// 查询单个指定数据

		// Ema表
		void		RecountEma(string codeId, ITimeType timeType);											// 重新计算某所有Ema, 会删除表全部重算
		void		RecountEmaFromTimePos(string codeId, ITimeType timeType, long timePos);					// 重新计算某品种指定时间点(毫秒)之后的所有Ema
		void		UpdateEma(string codeId, ITimeType timeType, IAvgValue ema);
		void		RemoveAllEmas(string codeId, ITimeType timeType);										// 删除某一品种的所有Ema表数据
		void		RemoveEmasByRange(string codeId, ITimeType timeType, long beginTime, long endTime);		// 按时间区间删除某一品种的ema表数据，区间为[beginTime,endTime)
		void		GetEmas(string codeId, ITimeType timeType, IQuery query,out IAvgValues emas);
		bool		GetOneEma(string codeId, ITimeType timeType, long timePos, out IAvgValue ema);			// 查询单个指定数据

		// macd表
		void		RecountMacd(string codeId, ITimeType timeType);											// 重新计算某品种所有macd, 会删除表重算
		void		RecountMacdFromTimePos(string codeId, ITimeType timeType, long timePos);				// 重新计算某品种指定时间点(毫秒)之后的所有macd
		void		UpdateMacd(string codeId, ITimeType timeType, IMacdValue macd);
		void		RemoveAllMacds(string codeId, ITimeType timeType);										// 删除某一品种的所有Ema表数据
		void		RemoveMacdsByRange(string codeId, ITimeType timeType, long beginTime, long endTime);	// 按时间区间删除某一品种的macd表数据，区间为[beginTime,endTime)
		void		GetMacds(string codeId, ITimeType timeType, IQuery query,out IMacdValues macds);
		bool		GetOneMacd(string codeId, ITimeType timeType, long timePos, out IMacdValue ma);			// 查询单个指定数据

		// divType表
		void		RecountDivType(string codeId, ITimeType timeType);										// 重新计算某品种所有divType, 会删除表重算
		void		RecountDivTypeFromTimePos(string codeId, ITimeType timeType, long timePos);				// 重新计算某品种指定时间点(毫秒)之后的所有divType
		void		UpdateDivType(string codeId, ITimeType timeType, IDivTypeValue divtype);
		void		RemoveAllDivTypes(string codeId, ITimeType timeType);										// 删除某一品种的所有Ema表数据
		void		RemoveDivTypesByRange(string codeId, ITimeType timeType, long beginTime, long endTime);		// 按时间区间删除某一品种的divType表数据，区间为[beginTime,endTime)
		void		GetDivTypes(string codeId, ITimeType timeType, IQuery query,out IDivTypeValues divtypes);
		bool		GetOneDivType(string codeId, ITimeType timeType, long timePos, out IDivTypeValue divtype);	// 查询单个指定数据

		// atr表
		void		RecountAtr(string codeId, ITimeType timeType);										    // 重新计算某品种所有Atr, 会删除表重算
		void		RecountAtrFromTimePos(string codeId, ITimeType timeType, long timePos);				    // 重新计算某品种指定时间点(毫秒)之后的所有Atr
		void		UpdateAtr(string codeId, ITimeType timeType, IAtrValue artValue);
		void		RemoveAllAtrs(string codeId, ITimeType timeType);										// 删除某一品种的所有Atr表数据
		void		RemoveAtrsByRange(string codeId, ITimeType timeType, long beginTime, long endTime);		// 按时间区间删除某一品种的Atr表数据，区间为[beginTime,endTime)
		void		GetAtrs(string codeId, ITimeType timeType, IQuery query,out IAtrValues avgAtrs);
		bool		GetOneAtr(string codeId, ITimeType timeType, long timePos, out IAtrValue avgAtr);	    // 查询单个指定数据

	};
};

#endif
