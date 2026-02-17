#include "DownloadData.h"
#include <iostream>
#include <Global.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_UnifyInterface.h>
#include <Factory_QDatabase.h>

void CDownloadData::Go()
{
	std::string cmd;
	std::cout << "input CodeId TimeType \n";
	getline(std::cin, cmd);
	printf("receive %s \n", cmd.c_str());

	std::vector<std::string> strs = CGlobal::SplitStr(cmd, " ");
	if (strs.size() != 2)
	{
		std::cout << "input error, strs.size() != 2, please input again \n";
		return;
	}
	std::string codeId = strs[0];
	std::string timeTypeStr = strs[1];
	Time_Type timeType = CTransToStr::Get_TimeType(timeTypeStr);
	UpdateDbKLineFromIB(codeId, timeType);
	return;

}

void CDownloadData::UpdateDbKLineFromIB(const std::string& codeId, Time_Type timeType)
{
	bool onlyRegularTime = false;
	time_t now = CGetRecordNo::GetRecordNo(timeType, Get_CurrentTime()->GetCurrentTime_millisecond());
	int days = 1;
	if (timeType == Time_Type::D1)
	{
		days = 5;
	}

	IBKLinePtrs klines = Get_IBApi()->QueryKLine(codeId, timeType, Get_CurrentTime()->GetCurrentTime_second(), days, onlyRegularTime, UseType::RealTrader);
	for (const auto& kline : klines)
	{
		if (kline->time >= now) continue;

		MakeAndGet_QDatabase()->UpdateKLine(codeId, timeType, kline);
	}
	// ����ָ��
	if (!klines.empty())
	{
		MakeAndGet_QDatabase()->UpdateAllIndexFromTimePos(codeId, timeType, klines[0]->time);
	}

	Log_Print(LogLevel::Info, fmt::format("update {}, {} {} klines",
		codeId.c_str(), CTransToStr::Get_TimeType(timeType).c_str(), klines.size()));

}
