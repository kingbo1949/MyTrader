#pragma once
#include "Calculator_MacdDiv.h"
class CContainer_MacdDiv
{
public:
	CContainer_MacdDiv() { ; };
	virtual ~CContainer_MacdDiv() { ; };

	virtual void					Initialize(Time_Type timeType, const IBKLinePtrs& klines) = 0;

	virtual void					Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData) = 0;

	virtual DivergenceType			GetMacdDivAtIndex(Time_Type timeType, size_t index) const = 0;

	// 内部会自动把time转换到与timeType匹配的时间戳
	virtual DivergenceType			GetMacdDivAtTime(Time_Type timeType, Tick_T time) const = 0;

	virtual Calculator_MacdDivPtr	GetMacdDivCalculator(Time_Type timeType) const = 0;
};
typedef std::shared_ptr<CContainer_MacdDiv> Container_MacdDivPtr;

