#include "stdafx.h"
#include "SaveData_Klines.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Global.h>
#include <Log.h>
#include <Factory_QDatabase.h>
CSaveData_Klines::CSaveData_Klines(const CodeStr& codeId, const KLine4Tables& kline4Tables)
	:m_codeId(codeId), m_kline4Tables(kline4Tables)
{
	IBKLinePtrs klines;
	for (auto kline4Table : m_kline4Tables)
	{
		klines.push_back(kline4Table.pkline);
	}
	m_pTradeDay = Make_TradeDayObj(klines);

}

void CSaveData_Klines::Go(const std::string& fileName)
{
	CLog::Instance()->DelLogFile(fileName);

	std::string klinetitle = GetTitle();
	CLog::Instance()->PrintStrToFile(fileName, klinetitle + "\n");

	std::string all_lineStr = "";
	for (size_t i = 0; i < m_kline4Tables.size(); ++i)
	{
		std::string str = GetKlineStr(m_kline4Tables[i]) + "\n";
		all_lineStr += str;

	}
	CLog::Instance()->PrintStrToFile(fileName, all_lineStr + "\n");

	return;


}




std::string CSaveData_Klines::GetTitle()
{
	return "time,open,high,low,close,vol,daySession,tradeDay,dif,dea,macd,divType,isUTurn,atr,ma5,ma20,ma60,ma200";
}

std::string CSaveData_Klines::GetKlineStr(const KLine4Table &kline4Table)
{
	bool daySession = CHighFrequencyGlobalFunc::IsDaySession(kline4Table.pkline->time);
	std::string daySessionStr = daySession ? "1" : "0";

	Tick_T tradeDay = 0;
	if (!m_pTradeDay->GetTradeDay(kline4Table.pkline->time, tradeDay))
	{
		tradeDay = CHighFrequencyGlobalFunc::GetDayMillisec(kline4Table.pkline->time) + 24 * 60 * 60 * 1000;
	}


	std::string temstr = fmt::format(
		"{},"					// 时间
		"{:.2f},"					// 开盘
		"{:.2f},"					// 最高
		"{:.2f},"					// 最低
		"{:.2f},"					// close
		"{},"						// vol
		"{},"						// daySession
		"{},"						// 交易日
		"{:.2f},"					// dif
		"{:.2f},"					// dea
		"{:.2f},"					// macd
		"{},"						// divType
		"{},"						// isUTurn
		"{:.2f}, "					// atr
		"{:.2f}, "					// ma5
		"{:.2f}, "					// ma20
		"{:.2f}, "					// ma60
		"{:.2f}"					// ma200
		,
		CGlobal::GetTickTimeStr(kline4Table.pkline->time).substr(0, 17).c_str(),			// 取时间字符串不要毫秒部分
		kline4Table.pkline->open,
		kline4Table.pkline->high,
		kline4Table.pkline->low,
		kline4Table.pkline->close,
		kline4Table.pkline->vol,
		daySessionStr.c_str(),
		CGlobal::GetTickTimeStr(tradeDay).substr(0, 8).c_str(),				// 仅取时间字符串日期部分
		kline4Table.pmacd->dif,
		kline4Table.pmacd->dea,
		kline4Table.pmacd->macd,
		CTransToStr::Get_DivergenceType(kline4Table.pDivType->divType).c_str(),
		kline4Table.pDivType->isUTurn ? "1" : "0",
		kline4Table.pAtr->avgAtr,
		kline4Table.pMa->v5,
		kline4Table.pMa->v20,
		kline4Table.pMa->v60,
		kline4Table.pMa->v200
	);

	return temstr;
}

