#pragma once
#include "Calculator_Macd.h"
class CContainer_Macd
{
public:
	CContainer_Macd() { ; };
	virtual ~CContainer_Macd() { ; };

	virtual void				Initialize(Time_Type timeType, const IBKLinePtrs& klines) = 0;

	virtual void				Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData) = 0;

	virtual MacdValue			GetMacdAtIndex(Time_Type timeType, int index) = 0;

	// 内部会自动把time转换到与timeType匹配的时间戳
	virtual MacdValue			GetMacdAtTime(Time_Type timeType, Tick_T time) = 0;

	virtual Calculator_MacdPtr	GetMacdCalculator(Time_Type timeType) = 0;
};
typedef std::shared_ptr<CContainer_Macd> Container_MacdPtr;


