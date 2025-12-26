#pragma once
#include "JSon_PricePair.h"
class CJSon_PricePair_Real : public CJSon_PricePair
{
public:
	CJSon_PricePair_Real() { ; };
	virtual ~CJSon_PricePair_Real() { ; };

	virtual void			Save_PricePairs(const std::map<StrategyIdHashId, PricePairPtr>& pricePairs) override final;

	virtual void			Load_PricePairs(std::map<StrategyIdHashId, PricePairPtr>& pricePairs) override final;

protected:
	std::string				GetFileName();
	Json::Value				MakeValue_PricePair_Item(StrategyIdHashId strategyIdHashId, PricePairPtr pPricePair);
	PricePairPtr			Make_PricePair_Item(const Json::Value& itemValue, StrategyIdHashId& strategyIdHashId);

	Json::Value				MakeValue_PricePairs(const std::map<StrategyIdHashId, PricePairPtr>& pricePairs);
	void 					Make_PricePairs(const Json::Value& pricePairsValue, std::map<StrategyIdHashId, PricePairPtr>& pricePairs);



};

