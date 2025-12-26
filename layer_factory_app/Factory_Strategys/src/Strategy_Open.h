#pragma once
#include "Strategy_Real.h"
class CStrategy_Open : public CStrategy_Real
{
public:
	CStrategy_Open(StrategyParamPtr	strategyParam);
	virtual ~CStrategy_Open() { ; };

	// 设置初始参数
	virtual void						SetTradeParams() override final;

	// 计算下单手数
	virtual double						MakeOrderVol() override final;


};

