#pragma once
#include "Container_Average.h"
class CContainer_Ema : public CContainer_Average
{
public:
	CContainer_Ema();
	virtual ~CContainer_Ema() { ; };

	virtual void					Initialize(Time_Type timeType, const IBKLinePtrs& klines) override final;

	virtual void					Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData) override final;

	virtual AverageValue			GetValueAtIndex(Time_Type timeType, int index) override final;

	// 内部会自动把time转换到与timeType匹配的时间戳
	virtual AverageValue			GetValueAtTime(Time_Type timeType, Tick_T time) override final;

	virtual Calculator_AveragePtr	GetAverageCalculator(Time_Type timeType) override final;

protected:
	std::map<Time_Type, Calculator_AveragePtr>			m_calculators;

};

