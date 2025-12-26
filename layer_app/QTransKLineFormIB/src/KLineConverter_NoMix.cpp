#include "KLineConverter_NoMix.h"

#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_QDatabase.h>
#include <Factory_UnifyInterface.h>
#include <Global.h>
#include "AppFuncs.h"

#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;



void CKLineConverter_NoMix::ConvertOneKLineFromIBToDb(const CodeStr& targetCodeId, TimePair timePair)
{
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(targetCodeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	if (contract->securityType == SecurityType::FUT)
	{
		// 期货需要15秒线
		//ConvertOneKLineFromIBToDb(targetCodeId, Time_Type::S15, timePair);
	}
	ConvertOneKLineFromIBToDb(targetCodeId, Time_Type::M1, timePair);
	ConvertOneKLineFromIBToDb(targetCodeId, Time_Type::M5, timePair);
	ConvertOneKLineFromIBToDb(targetCodeId, Time_Type::M15, timePair);
	ConvertOneKLineFromIBToDb(targetCodeId, Time_Type::M30, timePair);
	ConvertOneKLineFromIBToDb(targetCodeId, Time_Type::H1, timePair);
	ConvertOneKLineFromIBToDb(targetCodeId, Time_Type::D1, timePair);
}

void CKLineConverter_NoMix::ConvertOneKLineFromIBToDb(const CodeStr& codeId, Time_Type timeType, TimePair timePair)
{
	IBKLinePtrs klines = QueryFromIb(codeId, timeType, timePair);
	if (klines.empty()) return;

	CAppFuncs::UpdateToDb(codeId, timeType, klines);

	Log_Print(LogLevel::Info, fmt::format("{} {} kline size = {}, {} -- {}",
		codeId.c_str(),
		CTransToStr::Get_TimeType(timeType).c_str(),
		klines.size(),
		CGlobal::GetTickTimeStr(klines[0]->time).c_str(),
		CGlobal::GetTickTimeStr(klines.back()->time).c_str()
	));
}

IBKLinePtrs CKLineConverter_NoMix::QueryFromIb(const CodeStr& codeId, Time_Type timeType, TimePair timePair)
{
	Second_T beginSecond = timePair.beginPos / 1000;
	Second_T endSecond = timePair.endPos / 1000;

	bool onlyRegularTime = false;	// 盘前盘后数据都需要查询
	int daysForMinute = GetDayForIbQuery(timeType);

	// QTransKLineFormIB下载数据用于测试而不是实盘，使用UseType::Simulator
	IBKLinePtrs klines = Get_IBApi()->QueryKLineRange(codeId, timeType, beginSecond, endSecond, daysForMinute, onlyRegularTime, UseType::Simulator);
	return klines;
}

int CKLineConverter_NoMix::GetDayForIbQuery(Time_Type timeType)
{
	//if (timeType == Time_Type::M1) return 3;
	//return 5;
	if (timeType == Time_Type::S15) return 5;
	//if (timeType == Time_Type::S15) return 20;   // 15秒线可以查询20天，只不过等的时间会比较长
	//if (timeType == Time_Type::M1) return 30;
	if (timeType == Time_Type::M1) return 5;
	if (timeType == Time_Type::M5) return 60;

	return 75;
}



void CKLineConverter_NoMix::QueryKLineInDb(const CodeStr& codeId)
{
	QueryKLineInDb(codeId, Time_Type::M1);
	QueryKLineInDb(codeId, Time_Type::M5);
	QueryKLineInDb(codeId, Time_Type::M15);
	QueryKLineInDb(codeId, Time_Type::M30);
	QueryKLineInDb(codeId, Time_Type::H1);
	QueryKLineInDb(codeId, Time_Type::D1);


}

void CKLineConverter_NoMix::QueryKLineInDb(const CodeStr& codeId, Time_Type timeType)
{
	// 打印数据库中合约指定K线的简要描述
	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = 0;
	query.time_pair.endPos = Get_CurrentTime()->GetCurrentTime_millisecond();

	IBKLinePtrs klinesdb = MakeAndGet_QDatabase()->GetKLineByLoop(codeId, timeType, query.time_pair);
	if (klinesdb.empty())
	{
		Log_Print(LogLevel::Warn, fmt::format("{} {} kline in db size = {}",
			codeId.c_str(),
			CTransToStr::Get_TimeType(timeType).c_str(),
			klinesdb.size()
		));

	}
	else
	{
		Log_Print(LogLevel::Info, fmt::format("{} {} kline in db size = {}, {} -- {}",
			codeId.c_str(),
			CTransToStr::Get_TimeType(timeType).c_str(),
			klinesdb.size(),
			CGlobal::GetTickTimeStr(klinesdb[0]->time).c_str(),
			CGlobal::GetTickTimeStr(klinesdb.back()->time).c_str()
		));

	}

}

void CKLineConverter_NoMix::RequestKLineInDb(const CodeStr& codeId, Time_Type timeType, time_t beginPos, time_t endPos)
{
	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = beginPos;
	query.time_pair.endPos = endPos;

	IBKLinePtrs klinesdb = MakeAndGet_QDatabase()->GetKLineByLoop(codeId, timeType, query.time_pair);
	if (klinesdb.empty())
	{
		Log_Print(LogLevel::Warn, fmt::format("{} {} kline in db size = {}",
			codeId.c_str(),
			CTransToStr::Get_TimeType(timeType).c_str(),
			klinesdb.size()
		));

	}
	else
	{
		for (auto onekline : klinesdb)
		{
			Log_Print(LogLevel::Info, fmt::format("{} open = {}, close = {}, high = {}, low = {}",
				CGlobal::GetTickTimeStr(onekline->time).c_str(),
				onekline->open,
				onekline->close,
				onekline->high,
				onekline->low
			));
		}
	}


}


