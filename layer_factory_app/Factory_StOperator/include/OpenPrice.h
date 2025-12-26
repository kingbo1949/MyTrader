#pragma once
#include <base_struc.h>
class COpenPrice
{
public:
	COpenPrice() { ; };
	virtual ~COpenPrice() { ; };

	virtual bool			GetOpenPrice(StrategyIdHashId strategyIdHashId,  double& price) = 0;

	virtual void			SetOpenPrice(StrategyIdHashId strategyIdHashId, double price) = 0;

	virtual void			DelOpenPrice(StrategyIdHashId strategyIdHashId) = 0;
};
typedef std::shared_ptr<COpenPrice> OpenPricePtr;
