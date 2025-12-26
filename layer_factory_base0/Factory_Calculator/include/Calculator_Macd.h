#pragma once

#include <base_struc.h>
class CCalculator_Macd
{
public:
	CCalculator_Macd() { ; };
	virtual ~CCalculator_Macd() { ; };

	virtual void            Initialize(const IBKLinePtrs& klines) = 0;

	virtual void			Update(IBKLinePtr newKline, bool isNewData) = 0;

	virtual MacdValue		GetMacdAtIndex(size_t index) = 0;
	virtual MacdValue		GetMacdAtTime(Tick_T time) = 0;


	// [xBeginPos, xEndPos)内m_difValues的高低点
	virtual void			GetHighLow(int xBeginPos, int xEndPos, double& high, double& low) = 0;

};

typedef std::shared_ptr<CCalculator_Macd> Calculator_MacdPtr;