#pragma once
#include "Strategy_Real.h"
class CStrategy_Cover : public CStrategy_Real
{
public:
	CStrategy_Cover(StrategyParamPtr	strategyParam);
	virtual ~CStrategy_Cover() { ; };

	// 设置初始参数
	virtual void						SetTradeParams() override final;

	// 计算下单手数
	virtual double						MakeOrderVol() override final;


};

