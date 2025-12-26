#pragma once
#include "Calculator_Average.h"
class CContainer_Average
{
public:
	CContainer_Average() { ; };
	virtual ~CContainer_Average() { ; };

	virtual void					Initialize(Time_Type timeType, const IBKLinePtrs& klines) = 0;

	virtual void					Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData) = 0;

	virtual AverageValue			GetValueAtIndex(Time_Type timeType, int index) = 0;

	// 内部会自动把time转换到与timeType匹配的时间戳
	virtual AverageValue			GetValueAtTime(Time_Type timeType, Tick_T time) = 0;

	virtual Calculator_AveragePtr	GetAverageCalculator(Time_Type timeType) = 0;
};
typedef std::shared_ptr<CContainer_Average> Container_AveragePtr;


