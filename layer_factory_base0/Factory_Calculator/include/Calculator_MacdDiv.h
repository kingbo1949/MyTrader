#pragma once
#include <base_struc.h>
#include <optional>
class CCalculator_MacdDiv
{
public:
	CCalculator_MacdDiv() { ; };
	virtual ~CCalculator_MacdDiv() { ; };

	virtual void            Initialize(const IBKLinePtrs& klines) = 0;

	virtual void			Update(IBKLinePtr newKline, bool isNewData) = 0;

	virtual DivergenceType	GetMacdDivAtIndex(size_t index) const = 0;
	virtual DivergenceType	GetMacdDivAtTime(Tick_T time) const = 0;


};

typedef std::shared_ptr<CCalculator_MacdDiv> Calculator_MacdDivPtr;
