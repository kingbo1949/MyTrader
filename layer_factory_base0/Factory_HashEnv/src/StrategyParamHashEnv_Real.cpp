#include "pch.h"
#include "StrategyParamHashEnv_Real.h"

CStrategyParamHashEnv_Real::CStrategyParamHashEnv_Real(const StrategyParamPtrs& params)
{
	m_strategys_range.beginPos = 1;
	StrategyIdHashId i = 1;

	for (auto onestrategy : params)
	{
		m_strategys_range.endPos = i;
		m_valid_strategy[i] = onestrategy;
		++i;
	}
	return;

}

StrategyIdHashId CStrategyParamHashEnv_Real::Get_StrategyParam_Hash(const StrategyId& strategyId)
{
	for (StrategyIdHashId i = 1; i < StrategyHashIdSize; ++i)
	{
		if (m_valid_strategy[i])
		{
			if (m_valid_strategy[i]->key.ToStr() == strategyId) return i;
		}
		else
		{
			return 0;
		}
	}
	printf("Get_StrategyParam_Hash error \n");
	exit(-1);
	return 0;

}

StrategyParamPtr CStrategyParamHashEnv_Real::Get_StrategyParam(const StrategyIdHashId& strategyHashId)
{
	return m_valid_strategy[strategyHashId];
}

RangePair CStrategyParamHashEnv_Real::Get_StrategyId_Range()
{
	return m_strategys_range;
}
