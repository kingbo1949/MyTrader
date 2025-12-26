#pragma once
#include <base_struc.h>
class CCalculator_Average
{
public:
	CCalculator_Average() { ; };
	virtual ~CCalculator_Average() { ; };

	virtual void			Initialize(const IBKLinePtrs& klines) = 0;

	virtual void			Update(IBKLinePtr newKline, bool isNewData) = 0;

	virtual AverageValue	GetValueAtIndex(size_t index) = 0;

	virtual AverageValue	GetValueAtTime(Tick_T time) = 0;



};
typedef std::shared_ptr<CCalculator_Average> Calculator_AveragePtr;

