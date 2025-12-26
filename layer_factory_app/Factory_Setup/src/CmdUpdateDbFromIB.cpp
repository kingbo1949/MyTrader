#include "pch.h"
#include "CmdUpdateDbFromIB.h"

#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_UnifyInterface.h>
#include <Factory_QDatabase.h>
#include <Global.h>
#include "Factory_Setup.h"

void CCmdUpdateDbFromIB::operator()()
{
	UpdateDbKLineFromIB();
}
void CCmdUpdateDbFromIB::UpdateDbKLineFromIB()
{
	return;

	//IbContractPtrs contracts = GetStrategyCodeId(CodeIdType::ForAll);

	//for (auto contract : contracts)
	//{
	//	// UpdateDbKLineFromIB(contract->codeId, Time_Type::S15);
	//	UpdateDbKLineFromIB(contract->codeId, Time_Type::M1);
	//	UpdateDbKLineFromIB(contract->codeId, Time_Type::M5);
	//	UpdateDbKLineFromIB(contract->codeId, Time_Type::M15);
	//	UpdateDbKLineFromIB(contract->codeId, Time_Type::M30);
	//	UpdateDbKLineFromIB(contract->codeId, Time_Type::H1);
	//	UpdateDbKLineFromIB(contract->codeId, Time_Type::D1);

	//	Log_Print(LogLevel::Info, fmt::format("UpdateDbKLineFromIB {} over", contract->codeId.c_str()));
	//}


}

void CCmdUpdateDbFromIB::UpdateDbKLineFromIB(const std::string& codeId, Time_Type timeType)
{
	bool onlyRegularTime = false;
	time_t now = CGetRecordNo::GetRecordNo(timeType, Get_CurrentTime()->GetCurrentTime_millisecond());
	int days = 1;
	IBKLinePtrs klines = Get_IBApi()->QueryKLine(codeId, timeType, Get_CurrentTime()->GetCurrentTime_second(), days, onlyRegularTime, m_useType);
	for (const auto& kline : klines)
	{
		if (kline->time >= now) continue;

		MakeAndGet_QDatabase()->UpdateKLine(codeId, timeType, kline);
	}
	// 更新指标
	if (!klines.empty())
	{
		MakeAndGet_QDatabase()->RecountMaFromTimePos(codeId, timeType, klines[0]->time);
		//MakeAndGet_QDatabase()->RecountEmaFromTimePos(codeId, timeType, klines[0]->time);
		MakeAndGet_QDatabase()->RecountMacdFromTimePos(codeId, timeType, klines[0]->time);
		MakeAndGet_QDatabase()->RecountDivTypeFromTimePos(codeId, timeType, klines[0]->time);
	}

	Log_Print(LogLevel::Info, fmt::format("update {}, {} {} klines",
		codeId.c_str(), CTransToStr::Get_TimeType(timeType).c_str(), klines.size()));

}

