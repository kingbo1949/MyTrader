#include "QueryPosition.h"
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include <Factory_Log.h>
#include <Factory_IBJSon.h>

void CQueryPosition::GoQuery()
{
	QueryAllPs();

	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	for (auto strategy : strategyParams)
	{
		QueryPs(strategy);
	}
}

void CQueryPosition::QueryAllPs()
{
	PositionSizePtrs pss = MakeAndGet_DbPositionSize()->GetAll();
	for (auto ps : pss)
	{
		StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(ps->key);
		std::string strategyId = pStrategyParam->key.ToStr();

		// 单品种
		Log_Print(LogLevel::Info, fmt::format("{}:\t {} \t {}", strategyId.c_str(), GetPsVolStr(ps->vol, ps->longOrShort).c_str(), ps->avgPrice));

	}
	Log_Print(LogLevel::Info, "-------------------------------");

}

void CQueryPosition::QueryPs(StrategyParamPtr strategy)
{
	PositonSizeKey thisKey = Get_StrategyParamEnv()->Get_StrategyParam_Hash(strategy->key.ToStr());

	PositionSizePtr ps = MakeAndGet_DbPositionSize()->GetOne(thisKey);
	std::string psStr = "null";
	if (ps)
	{
		psStr = fmt::format("vol = {}, avgPrice = {}", ps->vol, ps->avgPrice);
	}

	Log_Print(LogLevel::Info, fmt::format("{} {}", strategy->key.ToStr(), psStr.c_str()));
}

std::string CQueryPosition::GetPsVolStr(double vol, LongOrShort longOrShort)
{
	std::string ret = "";
	if (longOrShort == LongOrShort::LongT)
	{
		ret = fmt::format("{}", vol);
	}
	else
	{
		ret = fmt::format("-{}", vol);
	}
	return ret;
}
