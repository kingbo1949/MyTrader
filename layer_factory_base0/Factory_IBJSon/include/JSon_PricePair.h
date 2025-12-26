#pragma once
#include "JSon.h"
class CJSon_PricePair : public CJSon
{
public:
	CJSon_PricePair() { ; };
	virtual ~CJSon_PricePair() { ; };

	virtual void			Save_PricePairs(const std::map<StrategyIdHashId, PricePairPtr>& pricePairs) = 0;

	virtual void			Load_PricePairs(std::map<StrategyIdHashId, PricePairPtr>& pricePairs) = 0;

};
typedef std::shared_ptr<CJSon_PricePair> JSon_PricePairPtr;


