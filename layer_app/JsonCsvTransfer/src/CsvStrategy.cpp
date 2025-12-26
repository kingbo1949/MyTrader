#include "CsvStrategy.h"


CCsvStrategy::CCsvStrategy()
{
}


CCsvStrategy::~CCsvStrategy()
{
}

StrategyParamPtrs CCsvStrategy::GetStrategyTotalParams(const StrategyTotalParamExs& paramExs)
{
	StrategyParamPtrs back;
	for (auto param : paramExs)
	{
		if (!param.pStrategyParam->canOpen && !param.pStrategyParam->canCover) continue;
		back.push_back(param.pStrategyParam);
	}
	return back;
}

void CCsvStrategy::GetStrategyTotalParamsForPs(const StrategyTotalParamExs& paramExs, StrategyTotalParamExs& output)
{
	for (auto param : paramExs)
	{
		if (param.psVol == 0) continue;
		output.push_back(param);
	}

}
