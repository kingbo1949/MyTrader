#pragma once
#include "Container_Macd.h"
class CContainer_Macd_Real : public CContainer_Macd
{
public:
	CContainer_Macd_Real();
	virtual ~CContainer_Macd_Real() { ; };

	virtual void				Initialize(Time_Type timeType, const IBKLinePtrs& klines) override final;

	virtual void				Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData) override final;

	virtual MacdValue			GetMacdAtIndex(Time_Type timeType, int index) override final;

	virtual MacdValue			GetMacdAtTime(Time_Type timeType, Tick_T time) override final;

	virtual Calculator_MacdPtr	GetMacdCalculator(Time_Type timeType) override final;


protected:
	std::map<Time_Type, Calculator_MacdPtr>			m_calculators;


};

