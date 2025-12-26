#include "stdafx.h"
#include "SaveData_Klines.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Global.h>
#include <Log.h>
#include <Factory_QDatabase.h>
CSaveData_Klines::CSaveData_Klines(const CodeStr& codeId, IBKLinePtrs klines)
	:m_codeId(codeId), m_klines(klines)
{
	m_pTradeDay = std::make_shared<CTradeDay>(klines);
}

void CSaveData_Klines::Go(const std::string& fileName)
{
	CLog::Instance()->DelLogFile(fileName);

	std::string klinetitle = GetTitle();
	CLog::Instance()->PrintStrToFile(fileName, klinetitle + "\n");

	std::string klineStr = "";
	for (size_t i = 0; i < m_klines.size(); ++i)
	{
		std::string str = GetKlineStr(m_klines[i]) + "\n";
		klineStr += str;

	}
	CLog::Instance()->PrintStrToFile(fileName, klineStr + "\n");

	return;


}




std::string CSaveData_Klines::GetTitle()
{
	return "time,open,high,low,close,vol,daySession,tradeDay";
}

std::string CSaveData_Klines::GetKlineStr(IBKLinePtr kline)
{
	bool daySession = CHighFrequencyGlobalFunc::IsDaySession(kline->time);
	std::string daySessionStr = daySession ? "1" : "0";

	Tick_T tradeDay = 0;
	if (!m_pTradeDay->GetTradeDay(kline->time, tradeDay))
	{
		tradeDay = CHighFrequencyGlobalFunc::GetDayMillisec(kline->time) + 24 * 60 * 60 * 1000;
	}


	std::string temstr = fmt::format(
		"{},"						// 时间
		"{:.2f},"					// 开盘
		"{:.2f},"					// 最高
		"{:.2f},"					// 最低
		"{:.2f},"					// close
		"{},"						// vol
		"{},"						// daySession
		"{}"						// 交易日
		,
		CGlobal::GetTickTimeStr(kline->time).substr(0, 17).c_str(),			// 取时间字符串不要毫秒部分
		kline->open,
		kline->high,
		kline->low,
		kline->close,
		kline->vol,
		daySessionStr.c_str(),
		CGlobal::GetTickTimeStr(tradeDay).substr(0, 8).c_str()				// 仅取时间字符串日期部分
	);

	return temstr;
}

