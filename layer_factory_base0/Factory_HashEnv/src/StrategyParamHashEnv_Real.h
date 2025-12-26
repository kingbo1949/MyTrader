#pragma once
#include "StrategyParamHashEnv.h"
class CStrategyParamHashEnv_Real : 	public CStrategyParamHashEnv
{
public:
	CStrategyParamHashEnv_Real(const StrategyParamPtrs& params);
	virtual ~CStrategyParamHashEnv_Real() { ; };

	virtual StrategyIdHashId		Get_StrategyParam_Hash(const StrategyId& strategyId) override final;

	virtual StrategyParamPtr		Get_StrategyParam(const StrategyIdHashId& strategyHashId) override final;

	virtual RangePair				Get_StrategyId_Range() override final;

protected:
	StrategyParamPtr				m_valid_strategy[StrategyHashIdSize] = { 0 };
	RangePair						m_strategys_range;		// 描述m_valid_strategy有效区间范围，它不是从下标零开始的


};

