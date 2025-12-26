#include "CFixQuote.h"
#include <Global.h>
#include "KLineConverter_NoMix.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_QDatabase.h>
#include "AppFuncs.h"
CFixQuote::CFixQuote(IbContractPtr contract, const TimePair& timePair)
	:m_contract(contract), m_timePair(timePair)
{
}

void CFixQuote::Go()
{
	FixOneType(Time_Type::M1);
	FixOneType(Time_Type::M5);
	FixOneType(Time_Type::M15);
	FixOneType(Time_Type::M30);
	FixOneType(Time_Type::H1);

	FixOneType(Time_Type::D1);

}


void CFixQuote::FixOneType(Time_Type timeType)
{
	CKLineConverter_NoMix klineConverter_nomix;
	IBKLinePtrs klines = klineConverter_nomix.QueryFromIb(m_contract->codeId, timeType, m_timePair);
	for (const auto& kline : klines)
	{
		std::string klinestr = CTransDataToStr::GetKlineStr_Csv(kline);
		Log_Print(LogLevel::Info, klinestr);
	}
	if (!klines.empty())
	{
		Log_Print(LogLevel::Info, fmt::format("{} {}-{}", 
			CTransToStr::Get_TimeType(timeType).c_str(),
			CGlobal::GetTickTimeStr(klines[0]->time).c_str(),
			CGlobal::GetTickTimeStr(klines.back()->time).c_str()
			));
	}
	if (CAppFuncs::CheckKLines(m_contract, klines))
	{
		
	}
	//CAppFuncs::UpdateToDb(m_contract->codeId, timeType, klines);

}
