#pragma once
#include "Container_MacdDiv.h"
#include "Container_Macd.h"
class CContainer_MacdDiv_Real : public CContainer_MacdDiv
{
public:
	CContainer_MacdDiv_Real(Container_MacdPtr);
	virtual ~CContainer_MacdDiv_Real() { ; };

	virtual void					Initialize(Time_Type timeType, const IBKLinePtrs& klines) override final;

	virtual void					Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData)  override final;

	virtual DivergenceType			GetMacdDivAtIndex(Time_Type timeType, size_t index) const  override final;

	virtual DivergenceType			GetMacdDivAtTime(Time_Type timeType, Tick_T time) const  override final;

	virtual Calculator_MacdDivPtr	GetMacdDivCalculator(Time_Type timeType) const  override final;

protected:
	std::map<Time_Type, Calculator_MacdDivPtr>			m_calculators;


};

