#include "FixOneDayQuote.h"
#include <Factory_Log.h>
#include <Global.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include "AppFuncs.h"
#include "KLineConverter_NoMix.h"
CFixOneDayQuote::CFixOneDayQuote(IbContractPtr contract, time_t dayTimeMs)
	:m_contract(contract), m_dayTimeMs(dayTimeMs)
{
	InitErrDaysInIBQuote();
}

void CFixOneDayQuote::Go()
{
	IBKLinePtr dayKlineDb = MakeAndGet_QDatabase()->GetOneKLine(m_contract->codeId, Time_Type::D1, m_dayTimeMs);

	m_dayKlineIb = QueryIbForDay();
	if (!m_dayKlineIb)
	{
		Log_Print(LogLevel::Err, "dayKlineIb is null");
		return;
	}

	bool canUpdate = true;

	IBKLinePtrs klines_M1 = QueryIb(Time_Type::M1);
	if (!CAppFuncs::CheckKLines(m_contract, Time_Type::M1, klines_M1, m_dayKlineIb))
	{
		canUpdate = false;
	}

	IBKLinePtrs klines_M5 = QueryIb(Time_Type::M5);
	if (!CAppFuncs::CheckKLines(m_contract, Time_Type::M5, klines_M5, m_dayKlineIb))
	{
		canUpdate = false;
	}

	IBKLinePtrs klines_M15 = QueryIb(Time_Type::M15);
	if (!CAppFuncs::CheckKLines(m_contract, Time_Type::M15, klines_M15, m_dayKlineIb))
	{
		canUpdate = false;
	}

	IBKLinePtrs klines_M30 = QueryIb(Time_Type::M30);
	if (!CAppFuncs::CheckKLines(m_contract, Time_Type::M30, klines_M30, m_dayKlineIb))
	{
		canUpdate = false;
	}

	IBKLinePtrs klines_H1 = QueryIb(Time_Type::H1);
	if (!CAppFuncs::CheckKLines(m_contract, Time_Type::H1, klines_H1, m_dayKlineIb))
	{
		canUpdate = false;
	}

	if (!canUpdate && !HaveErrInIBQuote(m_contract->codeId, m_dayTimeMs)) return;

	//CAppFuncs::UpdateToDb(m_contract->codeId, Time_Type::M1, klines_M1);
	//CAppFuncs::UpdateToDb(m_contract->codeId, Time_Type::M5, klines_M5);
	//CAppFuncs::UpdateToDb(m_contract->codeId, Time_Type::M15, klines_M15);
	CAppFuncs::UpdateToDb(m_contract->codeId, Time_Type::M30, klines_M30);
	CAppFuncs::UpdateToDb(m_contract->codeId, Time_Type::H1, klines_H1);
	MakeAndGet_QDatabase()->UpdateKLine(m_contract->codeId, Time_Type::D1, m_dayKlineIb);

	return;
}

IBKLinePtr CFixOneDayQuote::QueryIbForDay()
{
	TimePair timePair;
	timePair.beginPos = (m_dayTimeMs - 24 * 60 * 60 * 1000) + CHighFrequencyGlobalFunc::MakeMilliSecondPart("18:00:00", 0);
	timePair.endPos = m_dayTimeMs + CHighFrequencyGlobalFunc::MakeMilliSecondPart("18:00:00", 0);


	CKLineConverter_NoMix klineConverter_nomix;
	IBKLinePtrs klines = klineConverter_nomix.QueryFromIb(m_contract->codeId, Time_Type::D1, timePair);
	for (const auto& kline : klines)
	{
		std::string klinestr = CTransDataToStr::GetKlineStr_Csv(kline);
		Log_Print(LogLevel::Info, klinestr);
	}
	if (klines.size() != 1 || klines[0]->time != m_dayTimeMs)
	{
		return nullptr;
	}
	return klines[0];

}

IBKLinePtrs CFixOneDayQuote::QueryIb(Time_Type timeType)
{
	TimePair timePair;
	timePair.beginPos = (m_dayTimeMs - 24 * 60 * 60 * 1000) + CHighFrequencyGlobalFunc::MakeMilliSecondPart("18:00:00", 0);
	timePair.endPos = m_dayTimeMs + CHighFrequencyGlobalFunc::MakeMilliSecondPart("18:00:00", 0);


	CKLineConverter_NoMix klineConverter_nomix;
	IBKLinePtrs klines = klineConverter_nomix.QueryFromIb(m_contract->codeId, timeType, timePair);
	CHighFrequencyGlobalFunc::PrintKLines(klines);
	return klines;
}

void CFixOneDayQuote::InitErrDaysInIBQuote()
{
	time_t day = 0;

	std::string codeId = "NQ";
	// 20230313 11794.84->12220.69, M1:11793.67(20230313 08:49:00.0)->12219.48(20230313 02:54:00.0)
	day = CGlobal::GetTimeByStr("20230313 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;

	// 20230529 14332.50->14459.75, M1:14367.50(20230529 08:56:00.0)->14459.75(20230528 18:00:00.0)
	day = CGlobal::GetTimeByStr("20230529 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;

	// 20230612 14737.40->15013.58, M1:14734.39(20230611 18:00:00.0)->15010.51(20230612 16:43:00.0)
	day = CGlobal::GetTimeByStr("20230612 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;

	// 20230911 15494.22->15696.28, H1:15494.52(20230910 21:00:00.0)->15696.59(20230911 15:00:00.0)
	day = CGlobal::GetTimeByStr("20230911 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;

	// 20231211 16255.99->16461.15, M15:16255.17(20231211 06:15:00.0)->16460.31(20231211 16:45:00.0)
	day = CGlobal::GetTimeByStr("20231211 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;

	// 20240311 18134.65->18314.83, M1:18134.02(20240311 10:02:00.0)->18314.19(20240310 18:00:00.0)
	day = CGlobal::GetTimeByStr("20240311 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;

	codeId = "ES";
	// 20230911 4508.16->4542.78, M1:4508.43(20230910 21:36:00.0)->4543.06(20230911 09:34:00.0)
	day = CGlobal::GetTimeByStr("20230911 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;

	// 20230529 4213.25->4243.25, M1:4220.25(20230529 08:38:00.0)->4243.25(20230528 18:00:00.0)
	day = CGlobal::GetTimeByStr("20230529 00:00:00") * 1000;
	m_errDaysInIBQuote[codeId] = day;
}

bool CFixOneDayQuote::HaveErrInIBQuote(const std::string& codeId, time_t dayTimeMs)
{
	std::map<std::string, time_t>::const_iterator pos = m_errDaysInIBQuote.find(codeId);
	if (pos == m_errDaysInIBQuote.end()) return false;

	if (pos->second == dayTimeMs) return true;
	return false;

}
