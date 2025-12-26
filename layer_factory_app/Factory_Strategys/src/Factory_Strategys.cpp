#include "pch.h"
#include "Factory_Strategys.h"
#include "StrategyBase.h"
#include "StrategySuit.h"
#include <Factory_HashEnv.h>
#include <Factory_Log.h>
#include <Factory_Topics.h>

StrategySuitPtr g_strategy_suit[StrategyHashIdSize];	// 数组索引就是StrategyIdHashId
 
FACTORY_STRATEGYS_API void Make_RunTimeStrategys(const StrategyParamPtrs& params)
{
	size_t strategyCount = 0;

	for (auto oneparam : params)
	{
		//if (!oneparam->canOpen && !oneparam->canCover) continue;

		StrategySuitPtr pSuit = std::make_shared<CStrategySuit>(oneparam);
		pSuit->MakeSuit();

		strategyCount++;

		StrategyIdHashId index = Get_StrategyParamEnv()->Get_StrategyParam_Hash(oneparam->key.ToStr());
		g_strategy_suit[index] = pSuit;
	}
	Log_Print(LogLevel::Info, fmt::format("g_strategy_suit strategyCount = {}",
		strategyCount));

	return;

}

FACTORY_STRATEGYS_API void On_QuoteChg_Strategy(const CIBTick& tick, RoundAction openOrCover)
{
	static RangePair range = Get_StrategyParamEnv()->Get_StrategyId_Range();
	for (size_t i = range.beginPos; i <= range.endPos; ++i)
	{
		StrategySuitPtr onesuit = g_strategy_suit[i];
		for (const auto& onesub : onesuit->GetStrategySumModules())
		{
			onesub->OnRtnTick(tick.codeHash, openOrCover);
		}
	}
	return;
}

FACTORY_STRATEGYS_API void On_TradeChg_Strategy(StrategyIdHashId strategyIdHashId)
{
	StrategySuitPtr onesuit = g_strategy_suit[strategyIdHashId];

	for (const auto& onesub : onesuit->GetStrategySumModules())
	{
		onesub->OnRtnTrade(strategyIdHashId);
	}

	return;
}
