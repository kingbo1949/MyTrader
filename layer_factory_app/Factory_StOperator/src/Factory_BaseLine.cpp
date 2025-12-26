#include "pch.h"
#include "Factory_BaseLine.h"
#include <Factory_HashEnv.h>
#include "AppDefine.h"
#include "BaseLine_Channel.h"
#include "BaseLine_Manual.h"

std::map<StrategyIdHashId, BaseLinePtr> g_baseLines;
void Make_BaseLine(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint)
{
	StrategyIdHashId strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(pStrategyParam->key.ToStr());
	std::map<StrategyIdHashId, BaseLinePtr>::const_iterator pos = g_baseLines.find(strategyIdHashId);
	if (pos != g_baseLines.end())
	{
		printf("Make_BaseLine already have %s \n", pStrategyParam->key.ToStr().c_str());
		exit(-1);
	}

	BaseLinePtr pGetOpenBaseLine = nullptr;
	if (pStrategyParam->key.strategyName == DayBreakName)
	{
		pGetOpenBaseLine = std::make_shared<CBaseLine_Channel>(pStrategyParam, pContract, openPoint);
	}
	if (pStrategyParam->key.strategyName == DayBreakManualName)
	{
		pGetOpenBaseLine = std::make_shared<CBaseLine_Manual>(pStrategyParam, pContract, openPoint);
	}

	if (!pGetOpenBaseLine)
	{
		printf("Make_BaseLine fail %s \n", pStrategyParam->key.ToStr().c_str());
		exit(-1);
	}
	g_baseLines[strategyIdHashId] = pGetOpenBaseLine;
	return;

}
BaseLinePtr Get_BaseLine(StrategyIdHashId strategyIdHashId)
{
	std::map<StrategyIdHashId, BaseLinePtr>::const_iterator pos = g_baseLines.find(strategyIdHashId);
	if (pos == g_baseLines.end())
	{
		StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId);
		printf("Get_BaseLine fail %s \n", pStrategyParam->key.ToStr().c_str());
		exit(-1);
	}
	return pos->second;

}

HighLowPointOutsidePtr g_pHighLowPointOutside = nullptr;
HighLowPointOutsidePtr MakeAndGet_HighLowPointOutside()
{
	if (!g_pHighLowPointOutside)
	{
		g_pHighLowPointOutside = std::make_shared<CHighLowPointOutside>();
		g_pHighLowPointOutside->Init();

	}
	return g_pHighLowPointOutside;
}
