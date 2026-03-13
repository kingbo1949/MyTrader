#include "stdafx.h"
#include "SaveData_Klines.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Global.h>
#include <Log.h>
#include <Factory_QDatabase.h>
CSaveData_Klines::CSaveData_Klines(const CodeStr& codeId, const IBRichDataPtrs& richDatas)
	:m_codeId(codeId), m_richDatas(richDatas)
{

}

void CSaveData_Klines::Go(const std::string& fileName)
{
	CLog::Instance()->DelLogFile(fileName);

	std::string klinetitle = GetTitle();
	CLog::Instance()->PrintStrToFile(fileName, klinetitle + "\n");

	std::string all_lineStr = "";
	for (size_t i = 0; i < m_richDatas.size(); ++i)
	{
		std::string str = GetRichStr(m_richDatas[i]) + "\n";
		all_lineStr += str;

	}
	CLog::Instance()->PrintStrToFile(fileName, all_lineStr + "\n");

	return;


}




std::string CSaveData_Klines::GetTitle()
{
	return "time,open,high,low,close,vol,dif,dea,macd,divType,isUTurn,atr,ma5,ma20,ma60,ma200,preDayHigh,preDayLow";
}

std::string CSaveData_Klines::GetRichStr(const IBRichDataPtr rich)
{
	std::string temstr = fmt::format(
		"{},"						// 时间
		"{:.2f},"					// 开盘
		"{:.2f},"					// 最高
		"{:.2f},"					// 最低
		"{:.2f},"					// close
		"{},"						// vol
		"{:.2f},"					// dif
		"{:.2f},"					// dea
		"{:.2f},"					// macd
		"{},"						// divType
		"{},"						// isUTurn
		"{:.2f}, "					// atr
		"{:.2f}, "					// ma5
		"{:.2f}, "					// ma20
		"{:.2f}, "					// ma60
		"{:.2f}, "					// ma200
		"{:.2f}, "					// preHigh
		"{:.2f}"					// preLow
		,
		CGlobal::GetTickTimeStr(rich->time).substr(0, 17).c_str(),			// 取时间字符串不要毫秒部分
		rich->open,
		rich->high,
		rich->low,
		rich->close,
		rich->vol,
		rich->dif,
		rich->dea,
		rich->macd,
		CTransToStr::Get_DivergenceType(rich->divType).c_str(),
		rich->isUTurn ? "1" : "0",
		rich->avgAtr,
		rich->ma5,
		rich->ma20,
		rich->ma60,
		rich->ma200,
		rich->preDayHigh,
		rich->preDayLow
	);

	return temstr;
}

