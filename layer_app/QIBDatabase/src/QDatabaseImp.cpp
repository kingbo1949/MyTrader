#include "QDatabaseImp.h"
#include "Factory.h"

#include "Calculator/Calculator.h"
#include "MakeKlinePairs.h"
CQDatabaseImp::CQDatabaseImp()
	:m_updateCount(0), m_recentUpdateSecnd(0)
{
	m_pTickUpdator = std::make_shared<CTickUpdator>();
}

void CQDatabaseImp::UpdateTickToDB(ITick tick, const::Ice::Current& current)
{
	//printf("rec tick, %s, %s \n",
	//	CGlobal::GetTickTimeStr(tick.time).c_str(),
	//	tick.codeId.c_str()
	//	);
	m_pTickUpdator->UpdateTickToDB(tick);

	m_updateCount++;
	m_recentUpdateSecnd = time(nullptr);
	m_recentUpdateTick = tick;
	//printf("%d %s \n", m_updateCount, m_recentUpdateTick.codeId.c_str());
	return;
}

ITick CQDatabaseImp::GetLastUpdateTick(int& updateCount, long long int& recentUpdateSecnd, const::Ice::Current& current)
{
	updateCount = m_updateCount;
	recentUpdateSecnd = m_recentUpdateSecnd;
	return m_recentUpdateTick;
}

void CQDatabaseImp::RemoveAllTicks(::std::string codeId, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_TickHis()->RemoveKey(codeId);
}

void CQDatabaseImp::RemoveTicksByRange(::std::string codeId, long long int beginTime, long long int endTime, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_TickHis()->RemoveByRange(codeId, beginTime, endTime);
}


void CQDatabaseImp::GetTicks(::std::string codeId, IQuery query, ITicks& ticks, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_TickHis()->GetTicks(codeId, query, ticks);
}


bool CQDatabaseImp::GetOneTick(::std::string codeId, long long int timePos, ITick& tick, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_TickHis()->GetOne(codeId, timePos, tick);
}


void CQDatabaseImp::UpdateKLine(::std::string codeId, ITimeType timeType, IKLine kline, const::Ice::Current& current)
{
	if (!ValidKline(codeId, timeType, kline)) return;
	MakeAndGet_Env()->GetDB_KLine()->AddOne(codeId, timeType, kline);
}

void CQDatabaseImp::RemoveAllKLines(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_KLine()->RemoveKey(codeId, timeType);
}

void CQDatabaseImp::RemoveKLinesByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_KLine()->RemoveByRange(codeId, timeType, beginTime, endTime);
}

void CQDatabaseImp::RemoveOneKLine(::std::string codeId, ITimeType timeType, long long int timePos, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_KLine()->RemoveOne(codeId, timeType, timePos);
}

void CQDatabaseImp::GetKLines(::std::string codeId, ITimeType timeType, IQuery query, IKLines& klines, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timeType, query, klines);
}

bool CQDatabaseImp::GetOneKLine(::std::string codeId, ITimeType timeType, long long int timePos, IKLine& kline, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_KLine()->GetOne(codeId, timeType, timePos, kline);
}

void CQDatabaseImp::GetKLinePairs(::std::string first, ::std::string second, ITimeType timeType, IQuery query, IKLinePairs& kLinePairs, const::Ice::Current& current)
{
	CMakeKlinePairs makePairs(first, second, timeType, query);
	kLinePairs = makePairs.DoMake();
	return;

}

void CQDatabaseImp::GetInvalidKLines(::std::string codeId, ITimeType timeType, IKLines& klines, const::Ice::Current& current)
{
	IQuery query;
	query.byReqType = 1;	// 查询所有数据

	IKLines klineAll;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timeType, query, klineAll);
	for (const auto& kline : klineAll)
	{
		if (!ValidKline(codeId, timeType, kline))
		{
			klines.push_back(kline);
		}
	}
	return;
}


void CQDatabaseImp::RecountMa(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	RecountMa(codeId, timeType);
	return;
}

void CQDatabaseImp::RecountMaFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const::Ice::Current& current)
{
	RecountMaFromTimePos(codeId, timeType, timePos);
	return;
}


void CQDatabaseImp::UpdateMa(::std::string codeId, ITimeType timeType, IAvgValue ma, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ma()->AddOne(codeId, timeType, ma);
}


void CQDatabaseImp::RemoveAllMas(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ma()->RemoveKey(codeId, timeType);
}


void CQDatabaseImp::RemoveMasByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ma()->RemoveByRange(codeId, timeType, beginTime, endTime);
}

void CQDatabaseImp::GetMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& mas, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ma()->GetValues(codeId, timeType, query, mas);
}

bool CQDatabaseImp::GetOneMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ma, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_Ma()->GetOne(codeId, timeType, timePos, ma);
}

void CQDatabaseImp::RecountVwMa(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	RecountVwMa(codeId, timeType);
	return;

}

void CQDatabaseImp::RecountVwMaFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const::Ice::Current& current)
{
	RecountVwMaFromTimePos(codeId, timeType, timePos);
	return;

}

void CQDatabaseImp::UpdateVwMa(::std::string codeId, ITimeType timeType, IAvgValue vwma, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_VwMa()->AddOne(codeId, timeType, vwma);
}

void CQDatabaseImp::RemoveAllVwMas(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_VwMa()->RemoveKey(codeId, timeType);
}

void CQDatabaseImp::RemoveVwMasByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_VwMa()->RemoveByRange(codeId, timeType, beginTime, endTime);
}

void CQDatabaseImp::GetVwMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& vwmas, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_VwMa()->GetValues(codeId, timeType, query, vwmas);
}

bool CQDatabaseImp::GetOneVwMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& vwma, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_VwMa()->GetOne(codeId, timeType, timePos, vwma);
}

void CQDatabaseImp::RecountEma(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	RecountEma(codeId, timeType);
	return;

}

void CQDatabaseImp::RecountEmaFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const::Ice::Current& current)
{
	RecountEmaFromTimePos(codeId, timeType, timePos);
	return;
}

void CQDatabaseImp::UpdateEma(::std::string codeId, ITimeType timeType, IAvgValue ema, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ema()->AddOne(codeId, timeType, ema);
}

void CQDatabaseImp::RemoveAllEmas(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ema()->RemoveKey(codeId, timeType);
}

void CQDatabaseImp::RemoveEmasByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ema()->RemoveByRange(codeId, timeType, beginTime, endTime);
}


void CQDatabaseImp::GetEmas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& emas, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ema()->GetValues(codeId, timeType, query, emas);
}

bool CQDatabaseImp::GetOneEma(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ema, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_Ema()->GetOne(codeId, timeType, timePos, ema);
}

void CQDatabaseImp::RecountMacd(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	RecountMacd(codeId, timeType);
	return;

}

void CQDatabaseImp::RecountMacdFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const::Ice::Current& current)
{
	RecountMacdFromTimePos(codeId, timeType, timePos);
	return;

}

void CQDatabaseImp::UpdateMacd(::std::string codeId, ITimeType timeType, IMacdValue macd, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Macd()->AddOne(codeId, timeType, macd);
}

void CQDatabaseImp::RemoveAllMacds(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Macd()->RemoveKey(codeId, timeType);
}

void CQDatabaseImp::RemoveMacdsByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Macd()->RemoveByRange(codeId, timeType, beginTime, endTime);
}

void CQDatabaseImp::GetMacds(::std::string codeId, ITimeType timeType, IQuery query, IMacdValues& macds, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Macd()->GetValues(codeId, timeType, query, macds);
	return;
}

bool CQDatabaseImp::GetOneMacd(::std::string codeId, ITimeType timeType, long long int timePos, IMacdValue& macd, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_Macd()->GetOne(codeId, timeType, timePos, macd);
}

void CQDatabaseImp::RecountDivType(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	RecountDivType(codeId, timeType);
	return;

}

void CQDatabaseImp::RecountDivTypeFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const::Ice::Current& current)
{
	RecountDivTypeFromTimePos(codeId, timeType, timePos);
	return;
}

void CQDatabaseImp::UpdateDivType(::std::string codeId, ITimeType timeType, IDivTypeValue divtype, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_DivType()->AddOne(codeId, timeType, divtype);
}

void CQDatabaseImp::RemoveAllDivTypes(::std::string codeId, ITimeType timeType, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_DivType()->RemoveKey(codeId, timeType);
}

void CQDatabaseImp::RemoveDivTypesByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_DivType()->RemoveByRange(codeId, timeType, beginTime, endTime);
}

void CQDatabaseImp::GetDivTypes(::std::string codeId, ITimeType timeType, IQuery query, IDivTypeValues& divtypes, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_DivType()->GetValues(codeId, timeType, query, divtypes);
	return;
}

bool CQDatabaseImp::GetOneDivType(::std::string codeId, ITimeType timeType, long long int timePos, IDivTypeValue& divtype, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_DivType()->GetOne(codeId, timeType, timePos, divtype);
}

void CQDatabaseImp::RecountAtr(std::string codeId, ITimeType timeType, const Ice::Current &current)
{
	RecountAtr(codeId, timeType);
	return;

}

void CQDatabaseImp::RecountAtrFromTimePos(std::string codeId, ITimeType timeType, long long int timePos, const Ice::Current &current)
{
	RecountAtrFromTimePos(codeId, timeType, timePos);
	return;

}

void CQDatabaseImp::UpdateAtr(std::string codeId, ITimeType timeType, IAtrValue artValue, const Ice::Current &current)
{
	MakeAndGet_Env()->GetDB_Atr()->AddOne(codeId, timeType, artValue);
}

void CQDatabaseImp::RemoveAllAtrs(std::string codeId, ITimeType timeType, const Ice::Current &current)
{
	MakeAndGet_Env()->GetDB_Atr()->RemoveKey(codeId, timeType);
}

void CQDatabaseImp::RemoveAtrsByRange(std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const Ice::Current &current)
{
	MakeAndGet_Env()->GetDB_Atr()->RemoveByRange(codeId, timeType, beginTime, endTime);
}

void CQDatabaseImp::GetAtrs(std::string codeId, ITimeType timeType, IQuery query, IAtrValues &avgAtrs, 	const Ice::Current &current)
{
	MakeAndGet_Env()->GetDB_Atr()->GetValues(codeId, timeType, query, avgAtrs);
	return;

}

bool CQDatabaseImp::GetOneAtr(std::string codeId, ITimeType timeType, long long int timePos, IAtrValue &avgAtr, const Ice::Current &current)
{
	return MakeAndGet_Env()->GetDB_Atr()->GetOne(codeId, timeType, timePos, avgAtr);
}


void CQDatabaseImp::RecountMa(::std::string codeId, ITimeType timetype)
{
	MakeAndGet_Env()->GetDB_Ma()->RemoveKey(codeId, timetype);

	IKLines klines;
	GetKline_RecountQuery_All(codeId, timetype, klines);

	MakenAndGet_Calculator_Ma()->Initialize(codeId, timetype, klines);
	return;


}

void CQDatabaseImp::RecountVwMa(::std::string codeId, ITimeType timetype)
{
	MakeAndGet_Env()->GetDB_VwMa()->RemoveKey(codeId, timetype);

	IKLines klines;
	GetKline_RecountQuery_All(codeId, timetype, klines);

	MakenAndGet_Calculator_VwMa()->Initialize(codeId, timetype, klines);
	return;
}

void CQDatabaseImp::RecountEma(::std::string codeId, ITimeType timetype)
{
	MakeAndGet_Env()->GetDB_Ema()->RemoveKey(codeId, timetype);


	IKLines klines;
	GetKline_RecountQuery_All(codeId, timetype, klines);
	MakenAndGet_Calculator_Ema()->Initialize(codeId, timetype, klines);
	return;
}

void CQDatabaseImp::RecountMacd(::std::string codeId, ITimeType timetype)
{
	MakeAndGet_Env()->GetDB_Macd()->RemoveKey(codeId, timetype);

	IKLines klines;
	GetKline_RecountQuery_All(codeId, timetype, klines);
	MakenAndGet_Calculator_Macd()->Initialize(codeId, timetype, klines);
	return;
}

void CQDatabaseImp::RecountDivType(::std::string codeId, ITimeType timetype)
{
	MakeAndGet_Env()->GetDB_DivType()->RemoveKey(codeId, timetype);

	IKLines klines;
	GetKline_RecountQuery_All(codeId, timetype, klines);
	//printf("klines begin: %s \n", CGlobal::GetTickTimeStr(klines[0].time).c_str());
	//printf("klines end: %s \n", CGlobal::GetTickTimeStr(klines.back().time).c_str());

	MakenAndGet_Calculator_DivType()->Initialize(codeId, timetype, klines);
	return;
}

void CQDatabaseImp::RecountAtr(std::string codeId, ITimeType timetype)
{
	MakeAndGet_Env()->GetDB_Atr()->RemoveKey(codeId, timetype);

	IKLines klines;
	GetKline_RecountQuery_All(codeId, timetype, klines);
	//printf("klines begin: %s \n", CGlobal::GetTickTimeStr(klines[0].time).c_str());
	//printf("klines end: %s \n", CGlobal::GetTickTimeStr(klines.back().time).c_str());

	MakenAndGet_Calculator_Atr()->Initialize(codeId, timetype, klines);
	return;
}

void CQDatabaseImp::RecountMaFromTimePos(const::std::string& codeId, ITimeType timetype, long long int timePos)
{
	MakeAndGet_Env()->GetDB_Ma()->RemoveByRange(codeId, timetype, timePos, LLONG_MAX);

	IKLines klines;
	GetKline_RecountQuery_TimePos(codeId, timetype, timePos, klines);
	for (const auto& kline : klines)
	{
		MakenAndGet_Calculator_Ma()->Update(codeId, timetype, kline);
	}
	return;

}

void CQDatabaseImp::RecountVwMaFromTimePos(const::std::string& codeId, ITimeType timetype, long long int timePos)
{
	MakeAndGet_Env()->GetDB_VwMa()->RemoveByRange(codeId, timetype, timePos, LLONG_MAX);

	IKLines klines;
	GetKline_RecountQuery_TimePos(codeId, timetype, timePos, klines);
	for (const auto& kline : klines)
	{
		MakenAndGet_Calculator_VwMa()->Update(codeId, timetype, kline);
	}
	return;
}

void CQDatabaseImp::RecountEmaFromTimePos(const::std::string& codeId, ITimeType timetype, long long int timePos)
{
	MakeAndGet_Env()->GetDB_Ema()->RemoveByRange(codeId, timetype, timePos, LLONG_MAX);

	IKLines klines;
	GetKline_RecountQuery_TimePos(codeId, timetype, timePos, klines);
	for (const auto& kline : klines)
	{
		MakenAndGet_Calculator_Ema()->Update(codeId, timetype, kline);
	}
	return;
}

void CQDatabaseImp::RecountMacdFromTimePos(const::std::string& codeId, ITimeType timetype, long long int timePos)
{
	MakeAndGet_Env()->GetDB_Macd()->RemoveByRange(codeId, timetype, timePos, LLONG_MAX);

	IKLines klines;
	GetKline_RecountQuery_TimePos(codeId, timetype, timePos, klines);
	for (const auto& kline : klines)
	{
		MakenAndGet_Calculator_Macd()->Update(codeId, timetype, kline);
	}
	return;
}

void CQDatabaseImp::RecountDivTypeFromTimePos(const::std::string& codeId, ITimeType timetype, long long int timePos)
{
	MakeAndGet_Env()->GetDB_DivType()->RemoveByRange(codeId, timetype, timePos, LLONG_MAX);

	IKLines klines;
	GetKline_RecountQuery_TimePos(codeId, timetype, timePos, klines);
	for (const auto& kline : klines)
	{
		MakenAndGet_Calculator_DivType()->Update(codeId, timetype, kline);
	}
	return;
}

void CQDatabaseImp::RecountAtrFromTimePos(const std::string &codeId, ITimeType timetype, long long int timePos)
{
	MakeAndGet_Env()->GetDB_Atr()->RemoveByRange(codeId, timetype, timePos, LLONG_MAX);

	IKLines klines;
	GetKline_RecountQuery_TimePos(codeId, timetype, timePos, klines);
	for (const auto& kline : klines)
	{
		MakenAndGet_Calculator_Atr()->Update(codeId, timetype, kline);
	}
}

void CQDatabaseImp::GetKline_RecountQuery_All(const std::string& codeId, ITimeType timetype, IKLines& klines)
{
	// 在数据库内部，没有经过ICE，所以不需要接力
	IQuery query;
	query.byReqType = 1;	// 查询全部数据

	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timetype, query, klines);

	return;
}

void CQDatabaseImp::GetKline_RecountQuery_TimePos(const std::string& codeId, ITimeType timetype, long long int timePos, IKLines& klines)
{
	// 在数据库内部，没有经过ICE，所以不需要接力
	IQuery query;
	query.byReqType = 3;		// 查询某时间点之后
	query.tTime = timePos;
	query.dwSubscribeNum = 0;	// 为0表示查询某时点之后全部数据
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timetype, query, klines);
	return;
}



