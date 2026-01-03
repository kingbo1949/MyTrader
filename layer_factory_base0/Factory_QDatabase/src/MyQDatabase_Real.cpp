#include "pch.h"
#include "QDatabaseDefine.h"
#include "MyQDatabase_Real.h"
#include <QStruc.h>
#include <Global.h>
#include <MyMath.h>
#include "IceTransfor.h"
#include "Factory_IceProxy.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_HashEnv.h>
void CMyQDatabase_Real::UpdateTicks(IBTickPtr tick)
{
	ITick iTick = CIceTransfor::TransTickMyToIce(tick);

	// 更新到数据库
	MakeAndGet_IceProxy()->GetQDatabasePrx()->UpdateTickToDB(iTick);

	return;
}

IBTickPtr CMyQDatabase_Real::GetLastUpdateTick(int& updateCount, time_t& recentUpdateSecnd)
{
	Ice::Long recentUpdateSecndIce = 0;  // out 参数，初始化为 0 即可

	ITick recentUpdateTick =
		MakeAndGet_IceProxy()->GetQDatabasePrx()->GetLastUpdateTick(updateCount, recentUpdateSecndIce);

	recentUpdateSecnd = static_cast<time_t>(recentUpdateSecndIce);

	if (updateCount == 0) return nullptr;
	return CIceTransfor::TransTickIceToMy(recentUpdateTick);

}

IBTickPtrs CMyQDatabase_Real::GetTickHis(const CodeStr& codeId, const QQuery& query)
{
	IBTickPtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);

	}

	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);

	ITicks ticks;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetTicks(codeId, queryIce, ticks);

	for (auto& oneTick : ticks)
	{

		IBTickPtr tick = CIceTransfor::TransTickIceToMy(oneTick);
		if (!tick->codeHash) continue;

		back.push_back(tick);
	}


	return back;

}

IBTickPtrs CMyQDatabase_Real::GetTickHisByLoop(const CodeStr& codeId, const TimePair& timePair)
{
	QQuery query;
	query.query_type = QQueryType::AFTER_TIME;
	query.time_ms = timePair.beginPos;
	query.query_number = 5000;

	IBTickPtrs back;
	while (true)
	{
		IBTickPtrs ticks = GetTickHis(codeId, query);
		if (ticks.empty()) return back;

		for (const auto& onetick : ticks)
		{
			if (onetick->time >= timePair.endPos) return back;

			back.push_back(onetick);
		}
		query.time_ms = ticks.back()->time + 1;
	}

	return back;
}

IBTickPtr CMyQDatabase_Real::GetMarketQuoteDL(const CodeStr& codeId)
{
	IBTickPtr back = nullptr;

	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.query_number = 1;

	query.time_ms = Get_CurrentTime()->GetCurrentTime_millisecond() + 1;

	IBTickPtrs ticks = GetTickHis(codeId, query);
	if (ticks.size() == 0) return back;

	back = ticks.back();
	return back;

}

void CMyQDatabase_Real::RemoveAllTick(const CodeStr& codeId)
{
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllTicks(codeId);
}

IBTickPtr CMyQDatabase_Real::GetOneTick(const CodeStr& codeId, time_t timePos)
{
	ITick onetick;
	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneTick(codeId, timePos, onetick);
	if (!success) return nullptr;

	return CIceTransfor::TransTickIceToMy(onetick);
}

IBTickPtrs CMyQDatabase_Real::FliteTicks(const IBTickPtrs& ticks, time_t ms, int bigTick)
{
	IBTickPtrs back;
	IBTickPtr lasttick = nullptr;
	IBTickPtr lastAddtick = nullptr;

	for (const auto& tick : ticks)
	{
		if (!lasttick)
		{
			lasttick = tick;
			lastAddtick = tick;
			back.push_back(tick);
			continue;
		}
		if (ValidFlite(ms, bigTick, tick, lasttick, lastAddtick))
		{
			lastAddtick = tick;
			back.push_back(tick);
		}
		lasttick = tick;

	}
	return back;
}

void CMyQDatabase_Real::UpdateKLine(const CodeStr& codeId, Time_Type timeType, IBKLinePtr kline)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	IKLine ikline = CIceTransfor::TransKLineMyToIce(kline);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->UpdateKLine(codeId, iTimeType, ikline);
}

double CMyQDatabase_Real::MakeMa(const CodeStr& codeId, Time_Type timeType, int circle, Tick_T currentTime)
{
	Tick_T kline_time = CGetRecordNo::GetRecordNo(timeType, currentTime);

	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = kline_time - 1;		// 不包括指定时点
	query.query_number = circle;

	IBKLinePtrs klines = GetKLine(codeId, timeType, query);
	if (klines.size() < circle)
	{
		// 数据不充足，返回空集
		return  -1;
	}

	double sum = 0;
	for (auto kline : klines)
	{
		sum += kline->close;
	}
	return sum / circle;
}

IBKLinePtrs CMyQDatabase_Real::GetKLine(const CodeStr& codeId, Time_Type timeType, const QQuery& query)
{
	IBKLinePtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);

	}

	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IKLines klines;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetKLines(codeId, iTimeType, queryIce, klines);

	for (auto& oneKline : klines)
	{
		IBKLinePtr ibkline = CIceTransfor::TransKLineIceToMy(oneKline);

		back.push_back(ibkline);
	}


	return back;

}

IBKLinePtrs CMyQDatabase_Real::GetKLineByLoop(const CodeStr& codeId, Time_Type timeType, const TimePair& timePair)
{
	QQuery query;
	query.query_type = QQueryType::AFTER_TIME;
	query.time_ms = timePair.beginPos;
	query.query_number = 5000;

	IBKLinePtrs back;
	while (true)
	{
		IBKLinePtrs klines = GetKLine(codeId, timeType, query);
		if (klines.empty()) return back;

		for (const auto& onekline : klines)
		{
			if (onekline->time >= timePair.endPos) return back;

			back.push_back(onekline);
		}
		query.time_ms = klines.back()->time + 1;
	}

	return back;
}

void CMyQDatabase_Real::RemoveAllKLine(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllKLines(codeId, iTimeType);
}

void CMyQDatabase_Real::RemoveKLines(const CodeStr& codeId, Time_Type timeType, const TimePair& timePair)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveKLinesByRange(codeId, iTimeType, timePair.beginPos, timePair.endPos);
	return;

}

void CMyQDatabase_Real::RemoveOneKLine(const CodeStr& codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveOneKLine(codeId, iTimeType, timePos);
	return;
}

IBKLinePtr CMyQDatabase_Real::GetOneKLine(const CodeStr& codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IKLine onekline;

	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneKLine(codeId, iTimeType, timePos, onekline);
	if (!success) return nullptr;

	return CIceTransfor::TransKLineIceToMy(onekline);

}

IBKLinePairs CMyQDatabase_Real::GetKLinePair(const CodeStr& firstcode, const CodeStr& secondcode, Time_Type timeType, const QQuery& query)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);

	IKLinePairs klinePairs;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetKLinePairs(firstcode, secondcode, iTimeType, queryIce, klinePairs);

	IBKLinePairs ret;
	for (const auto& onePair : klinePairs)
	{
		IBKLinePair ibPair = CIceTransfor::TransKLinePairIceToMy(onePair);
		ret.push_back(ibPair);
	}
	return ret;
}

IBKLinePtrs CMyQDatabase_Real::GetInvalidKLines(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IKLines klines;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetInvalidKLines(codeId, iTimeType, klines);

	IBKLinePtrs back;
	for (auto& oneKline : klines)
	{
		IBKLinePtr ibkline = CIceTransfor::TransKLineIceToMy(oneKline);

		back.push_back(ibkline);
	}
	return back;
}

void CMyQDatabase_Real::RecountMa(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountMa(codeId, iTimeType);
	return;
}

void CMyQDatabase_Real::RecountMaFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountMaFromTimePos(codeId, iTimeType, beginTime);
	return;
}

IBMaPtrs CMyQDatabase_Real::GetMas(const CodeStr& codeId, Time_Type timeType, const QQuery& query)
{
	IBMaPtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);

	}

	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAvgValues values;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetMas(codeId, iTimeType, queryIce, values);

	for (auto& oneMa : values)
	{
		IBMaPtr ibMa = CIceTransfor::TransAvgValueIceToMy(oneMa);

		back.push_back(ibMa);
	}


	return back;

}

void CMyQDatabase_Real::RemoveAllMas(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllMas(codeId, iTimeType);
	return;

}

void CMyQDatabase_Real::RemoveMasByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveMasByRange(codeId, iTimeType, beginTime, endTime);
}

IBMaPtr CMyQDatabase_Real::GetOneMa(const CodeStr& codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAvgValue oneValue;
	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneMa(codeId, iTimeType, timePos, oneValue);
	if (!success) return nullptr;

	return CIceTransfor::TransAvgValueIceToMy(oneValue);

}

void CMyQDatabase_Real::RecountVwMa(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountVwMa(codeId, iTimeType);
	return;

}

void CMyQDatabase_Real::RecountVwMaFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountVwMaFromTimePos(codeId, iTimeType, beginTime);
	return;

}

IBVwMaPtrs CMyQDatabase_Real::GetVwMas(const CodeStr& codeId, Time_Type timeType, const QQuery& query)
{
	IBVwMaPtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);

	}

	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAvgValues values;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetVwMas(codeId, iTimeType, queryIce, values);

	for (auto& oneVwMa : values)
	{
		IBVwMaPtr ibMa = CIceTransfor::TransAvgValueIceToMy(oneVwMa);

		back.push_back(ibMa);
	}


	return back;
}

void CMyQDatabase_Real::RemoveAllVwMas(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllVwMas(codeId, iTimeType);
	return;

}

void CMyQDatabase_Real::RemoveVwMasByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveVwMasByRange(codeId, iTimeType, beginTime, endTime);

}

IBVwMaPtr CMyQDatabase_Real::GetOneVwMa(const CodeStr& codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAvgValue oneValue;
	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneVwMa(codeId, iTimeType, timePos, oneValue);
	if (!success) return nullptr;

	return CIceTransfor::TransAvgValueIceToMy(oneValue);
}

void CMyQDatabase_Real::RecountEma(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountEma(codeId, iTimeType);
	return;
}

void CMyQDatabase_Real::RecountEmaFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountEmaFromTimePos(codeId, iTimeType, beginTime);
	return;
}

IBEmaPtrs CMyQDatabase_Real::GetEmas(const CodeStr& codeId, Time_Type timeType, const QQuery& query)
{
	IBEmaPtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);
	}
	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAvgValues values;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetEmas(codeId, iTimeType, queryIce, values);

	for (auto& oneEma : values)
	{
		IBEmaPtr ibMa = CIceTransfor::TransAvgValueIceToMy(oneEma);

		back.push_back(ibMa);
	}


	return back;

}
void CMyQDatabase_Real::RemoveAllEmas(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllEmas(codeId, iTimeType);
	return;

}

void CMyQDatabase_Real::RemoveEmasByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveEmasByRange(codeId, iTimeType, beginTime, endTime);
}

IBEmaPtr CMyQDatabase_Real::GetOneEma(const CodeStr& codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAvgValue oneValue;
	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneMa(codeId, iTimeType, timePos, oneValue);
	if (!success) return nullptr;

	return CIceTransfor::TransAvgValueIceToMy(oneValue);

}

void CMyQDatabase_Real::RecountMacd(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountMacd(codeId, iTimeType);
}

void CMyQDatabase_Real::RecountMacdFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountMacdFromTimePos(codeId, iTimeType, beginTime);
}

IBMacdPtrs CMyQDatabase_Real::GetMacds(const CodeStr& codeId, Time_Type timeType, const QQuery& query)
{
	IBMacdPtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);
	}
	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IMacdValues values;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetMacds(codeId, iTimeType, queryIce, values);

	for (auto& oneMacd : values)
	{
		IBMacdPtr pMacd = CIceTransfor::TransMacd(oneMacd);

		back.push_back(pMacd);
	}
	return back;

}

void CMyQDatabase_Real::RemoveAllMacds(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllMacds(codeId, iTimeType);
}

void CMyQDatabase_Real::RemoveMacdsByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveMacdsByRange(codeId, iTimeType, beginTime, endTime);
}

IBMacdPtr CMyQDatabase_Real::GetOneMacd(const CodeStr& codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IMacdValue oneValue;
	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneMacd(codeId, iTimeType, timePos, oneValue);
	if (!success) return nullptr;

	return CIceTransfor::TransMacd(oneValue);
}

void CMyQDatabase_Real::RecountDivType(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountDivType(codeId, iTimeType);
}

void CMyQDatabase_Real::RecountDivTypeFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountDivTypeFromTimePos(codeId, iTimeType, beginTime);
}

IBDivTypePtrs CMyQDatabase_Real::GetDivTypes(const CodeStr& codeId, Time_Type timeType, const QQuery& query)
{
	IBDivTypePtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);
	}
	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IDivTypeValues values;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetDivTypes(codeId, iTimeType, queryIce, values);

	for (auto& oneDivType : values)
	{
		IBDivTypePtr pDivType = CIceTransfor::TransDivType(oneDivType);

		back.push_back(pDivType);
	}
	return back;
}

void CMyQDatabase_Real::RemoveAllDivTypes(const CodeStr& codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllDivTypes(codeId, iTimeType);

}

void CMyQDatabase_Real::RemoveDivTypesByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveDivTypesByRange(codeId, iTimeType, beginTime, endTime);
}

IBDivTypePtr CMyQDatabase_Real::GetOneDivType(const CodeStr& codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IDivTypeValue oneValue;
	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneDivType(codeId, iTimeType, timePos, oneValue);
	if (!success) return nullptr;

	return CIceTransfor::TransDivType(oneValue);

}

void CMyQDatabase_Real::RecountAtr(const CodeStr &codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountAtr(codeId, iTimeType);

}

void CMyQDatabase_Real::RecountAtrFromTimePos(const CodeStr &codeId, Time_Type timeType, Tick_T beginTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RecountAtrFromTimePos(codeId, iTimeType, beginTime);

}

IBAtrPtrs CMyQDatabase_Real::GetAtrs(const CodeStr &codeId, Time_Type timeType, const QQuery &query)
{
	IBAtrPtrs back;

	if (Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str()) == 0)
	{
		Log_Print(LogLevel::Err, fmt::format("codeId = {} can not get hashid", codeId.c_str()));
		exit(-1);
	}
	IQuery queryIce = CIceTransfor::TransQueryMyToIce(query);
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAtrValues values;
	MakeAndGet_IceProxy()->GetQDatabasePrx()->GetAtrs(codeId, iTimeType, queryIce, values);

	for (auto& oneAtr : values)
	{
		IBAtrPtr pAtr = CIceTransfor::TransAtr(oneAtr);

		back.push_back(pAtr);
	}
	return back;

}

void CMyQDatabase_Real::RemoveAllAtrs(const CodeStr &codeId, Time_Type timeType)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAllAtrs(codeId, iTimeType);

}

void CMyQDatabase_Real::RemoveAtrsByRange(const CodeStr &codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);
	MakeAndGet_IceProxy()->GetQDatabasePrx()->RemoveAtrsByRange(codeId, iTimeType, beginTime, endTime);

}

IBAtrPtr CMyQDatabase_Real::GetOneAtr(const CodeStr &codeId, Time_Type timeType, time_t timePos)
{
	ITimeType iTimeType = CIceTransfor::TransTimeTypeToIce(timeType);

	IAtrValue oneValue;
	bool success = MakeAndGet_IceProxy()->GetQDatabasePrx()->GetOneAtr(codeId, iTimeType, timePos, oneValue);
	if (!success) return nullptr;

	return CIceTransfor::TransAtr(oneValue);

}

bool CMyQDatabase_Real::ValidFlite(time_t ms, int bigTick, IBTickPtr thisTick, IBTickPtr lastTick, IBTickPtr lastAddedTick)
{
	if (thisTick->time - lastAddedTick->time >= ms)
	{
		//printf("add for ms \n");
		return true;
	}
	if (thisTick->bidAsks[0].bid - lastTick->bidAsks[0].bid >= bigTick)
	{
		//printf("add for bid \n");
		return true;
	}
	if (lastTick->bidAsks[0].ask - thisTick->bidAsks[0].ask >= bigTick)
	{
		//printf("add for ask \n");
		return true;
	}


	return false;
}

