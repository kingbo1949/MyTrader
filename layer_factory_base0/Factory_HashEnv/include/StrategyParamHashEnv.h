#pragma once
#include <base_trade.h>
class CStrategyParamHashEnv
{
public:
	CStrategyParamHashEnv() { ; };
	virtual ~CStrategyParamHashEnv() { ; };

	virtual StrategyIdHashId		Get_StrategyParam_Hash(const StrategyId& strategyId) = 0;

	virtual StrategyParamPtr		Get_StrategyParam(const StrategyIdHashId& strategyHashId) = 0;

	virtual RangePair				Get_StrategyId_Range() = 0;

};
typedef std::shared_ptr<CStrategyParamHashEnv> StrategyParamHashEnvPtr;
