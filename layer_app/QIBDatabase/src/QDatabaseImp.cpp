#include "QDatabaseImp.h"
#include "Factory.h"
#include <Factory_Log.h>
#include <Global.h>
#include "Calculator/Calculator.h"
#include "MakeKlinePairs.h"
#include "../cmd/Cmd_RecountAllIndex.h"
#include "../cmd/Cmd_UpdateAllIndexFromTimePos.h"

CQDatabaseImp::CQDatabaseImp()
	:m_updateCount(0), m_recentUpdateSecnd(0)
{
	m_pTickUpdator = std::make_shared<CTickUpdator>();
}

int CQDatabaseImp::IdlCount(const Ice::Current &current)
{
	return MakeAndGet_MyThreadPool()->idlCount();
}

bool CQDatabaseImp::IsAllIdle(const Ice::Current &current)
{
	return MakeAndGet_MyThreadPool()->isAllIdle();
}
int CQDatabaseImp::TaskCount(const ::Ice::Current& current)
{
	return MakeAndGet_MyThreadPool()->taskCount();
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

void CQDatabaseImp::DelCodeId(std::string codeId, long long int beginTime, long long int endTime, const Ice::Current &current)
{
	MakeAndGet_Env()->GetDB_TickHis()->RemoveByRange(codeId, beginTime, endTime);

	for (auto timetype : ALL_TIME_TYPES)
	{
		MakeAndGet_Env()->GetDB_DivType()->RemoveByRange(codeId, timetype, beginTime, endTime);
		MakeAndGet_Env()->GetDB_Macd()->RemoveByRange(codeId, timetype, beginTime, endTime);
		MakeAndGet_Env()->GetDB_Ma()->RemoveByRange(codeId, timetype, beginTime, endTime);
		MakeAndGet_Env()->GetDB_Ema()->RemoveByRange(codeId, timetype, beginTime, endTime);
		MakeAndGet_Env()->GetDB_VwMa()->RemoveByRange(codeId, timetype, beginTime, endTime);
		MakeAndGet_Env()->GetDB_Atr()->RemoveByRange(codeId, timetype, beginTime, endTime);
		MakeAndGet_Env()->GetDB_KLine()->RemoveByRange(codeId, timetype, beginTime, endTime);

	}
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
	// Log_Print(LogLevel::Info, fmt::format("{}", GetKlineStr(kline).c_str() ));
}

void CQDatabaseImp::UpdateKLines(std::string codeId, ITimeType timeType, IKLines klines, const Ice::Current &current)
{
	IKLines temKines;
	for (const auto& kline : klines)
	{
		if (!ValidKline(codeId, timeType, kline)) continue;
		temKines.push_back(kline);
	}
	MakeAndGet_Env()->GetDB_KLine()->AddSome(codeId, timeType, temKines);

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

bool CQDatabaseImp::GetLastDayKLine(std::string codeId, long long int msTime, IKLine &kline, const Ice::Current &current)
{
	// 得到机器时间day
	time_t seconds = msTime / 1000;
	time_t msDay = CGlobal::QGetDayTime(seconds) * 1000;

	time_t belongDay = 0;
	if (msTime < msDay + 17 * 60 * 60 * 1000)
	{
		// msTime是一个小于17:00:00的时间
		belongDay = msTime;
	}else
	{
		// msTime是一个17:00:00 - 24:00:00 的时间
		belongDay = msTime + 24 * 60 * 60 * 1000;
	}
	IQuery query;
	query.byReqType = 2;		// 2表示请求某个时间以前(包括该时间)多少个单位的数据(dwSubscribeNum为0时表示该时间前所有的数据)
	query.tTime = belongDay -1; // 时间(请求类型为2、3时使用)(毫秒) 数据区间为[... : tTime] 或者 [tTime : ...]
	query.dwSubscribeNum = 1;

	IKLines klines;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, ITimeType::D1, query, klines);

	if (klines.size() != 1) return false;
	kline = klines[0];
	return true;
}

void CQDatabaseImp::RecountAllIndex(std::string codeId, ITimeType timeType, const Ice::Current &current)
{
	MakeAndGet_MyThreadPool()->commit(0,CCmd_RecountAllIndex(codeId, timeType));
}

void CQDatabaseImp::UpdateAllIndexFromTimePos(std::string codeId, ITimeType timeType, long long int timePos, const Ice::Current &current)
{
	MakeAndGet_MyThreadPool()->commit(0,CCmd_UpdateAllIndexFromTimePos(codeId, timeType, timePos));
}


void CQDatabaseImp::GetMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& mas, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ma()->GetValues(codeId, timeType, query, mas);
}

bool CQDatabaseImp::GetOneMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ma, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_Ma()->GetOne(codeId, timeType, timePos, ma);
}

void CQDatabaseImp::GetVwMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& vwmas, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_VwMa()->GetValues(codeId, timeType, query, vwmas);
}

bool CQDatabaseImp::GetOneVwMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& vwma, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_VwMa()->GetOne(codeId, timeType, timePos, vwma);
}

void CQDatabaseImp::GetEmas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& emas, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_Ema()->GetValues(codeId, timeType, query, emas);
}

bool CQDatabaseImp::GetOneEma(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ema, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_Ema()->GetOne(codeId, timeType, timePos, ema);
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

void CQDatabaseImp::GetDivTypes(::std::string codeId, ITimeType timeType, IQuery query, IDivTypeValues& divtypes, const::Ice::Current& current)
{
	MakeAndGet_Env()->GetDB_DivType()->GetValues(codeId, timeType, query, divtypes);
	return;
}

bool CQDatabaseImp::GetOneDivType(::std::string codeId, ITimeType timeType, long long int timePos, IDivTypeValue& divtype, const::Ice::Current& current)
{
	return MakeAndGet_Env()->GetDB_DivType()->GetOne(codeId, timeType, timePos, divtype);
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






