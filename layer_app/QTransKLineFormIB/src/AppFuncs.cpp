#include "AppFuncs.h"
#include <Global.h>
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
bool CAppFuncs::CheckKLines(IbContractPtr contract, const IBKLinePtrs& klines)
{
	bool ret = true;
	for (const auto& kline : klines)
	{
		if (CGlobal::DToI(kline->high / contract->minMove) == CGlobal::DToI(kline->low / contract->minMove))
		{
			std::string klinestr = CTransDataToStr::GetKlineStr_Csv(kline);
			Log_Print(LogLevel::Err, fmt::format("kline err, {}", klinestr.c_str()));
			ret = false;
		}
	}

	return ret;
}

bool CAppFuncs::CheckKLines(IbContractPtr contract, Time_Type timeType, const IBKLinePtrs& klines, IBKLinePtr dayKLine)
{
	bool ret = true;
	HighAndLow highLow = CHighFrequencyGlobalFunc::MakeHighAndLow(klines, false);
	if (CGlobal::DToI(highLow.high / contract->minMove) != CGlobal::DToI(dayKLine->high / contract->minMove))
	{
		ret = false;
	}
	if (CGlobal::DToI(highLow.low / contract->minMove) != CGlobal::DToI(dayKLine->low / contract->minMove))
	{
		ret = false;
	}
	if (!ret)
	{
		PrintErr_HighLow_Klines(timeType, highLow, dayKLine);
	}
	return ret;
}

void CAppFuncs::PrintErr_HighLow_Klines(Time_Type timeType, const HighAndLow& klinesHighLow, IBKLinePtr dayKLine)
{
	std::string highlowStr = fmt::format("{:.2f}({})->{:.2f}({})",
		klinesHighLow.low,
		klinesHighLow.lowPosStr.c_str(),
		klinesHighLow.high,
		klinesHighLow.highPosStr.c_str()
	);

	std::string str = fmt::format("{} {:.2f}->{:.2f}, {}:{}",
		CGlobal::GetTickTimeStr(dayKLine->time).substr(0, 8).c_str(),
		dayKLine->low,
		dayKLine->high,
		CTransToStr::Get_TimeType(timeType).c_str(),
		highlowStr.c_str()
	);
	Log_Print(LogLevel::Warn, str);
}

void CAppFuncs::UpdateToDb(const std::string& codeId, Time_Type timeType, const IBKLinePtrs& klines)
{
	if (!klines.empty())
	{
		TimePair timePair;
		timePair.beginPos = klines[0]->time;
		timePair.endPos = klines.back()->time + 1;
		MakeAndGet_QDatabase()->RemoveKLines(codeId, timeType, timePair);

	}

	for (auto onekline : klines)
	{
		//std::string klinestr = fmt::format("{} open = {:.2f}, high = {:.2f}, low = {:.2f}, close = {:.2f}",
		//	CGlobal::GetTickTimeStr(onekline->time).c_str(),
		//	onekline->open,
		//	onekline->high,
		//	onekline->low,
		//	onekline->close

		//);
		//Log_Print(LogLevel::Info, klinestr);
		MakeAndGet_QDatabase()->UpdateKLine(codeId, timeType, onekline);
	}
	return;

}

void CAppFuncs::DelToDb(const std::string& codeId, Time_Type timeType, const TimePair& timePair)
{
	MakeAndGet_QDatabase()->RemoveKLines(codeId, timeType, timePair);

	MakeAndGet_QDatabase()->RecountAllIndex(codeId, timeType);

	return;
	
}


