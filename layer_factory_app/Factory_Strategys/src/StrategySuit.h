#pragma once


// 一个CStrategySuit，就是StrategyParams.csv中的一行，它包括开平等子模块

#include <base_trade.h>
#include "StrategyBase.h"
class CStrategySuit
{
public:
	CStrategySuit(StrategyParamPtr	strategyParam);
	virtual ~CStrategySuit() { ; };

	void					MakeSuit();

	StrategyBasePtrs		GetStrategySumModules();


protected:
	StrategyParamPtr		m_strategyParam;
	StrategyBasePtrs		m_subModules;






};
typedef std::shared_ptr<CStrategySuit> StrategySuitPtr;
