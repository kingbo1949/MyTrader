#include "pch.h"
#include "JSon_PricePair_Real.h"
#include <Factory_HashEnv.h>
void CJSon_PricePair_Real::Save_PricePairs(const std::map<StrategyIdHashId, PricePairPtr>& pricePairs)
{
	Json::Value dealsValue = MakeValue_PricePairs(pricePairs);
	SaveJSonValue(GetFileName(), dealsValue);

}
void CJSon_PricePair_Real::Load_PricePairs(std::map<StrategyIdHashId, PricePairPtr>& pricePairs)
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value pricePairsValue;
	LoadJSonValue(GetFileName(), pricePairsValue);

	Make_PricePairs(pricePairsValue, pricePairs);

	return;

}
std::string CJSon_PricePair_Real::GetFileName()
{
	return  "./db/pricePairs.json";
}
Json::Value CJSon_PricePair_Real::MakeValue_PricePair_Item(StrategyIdHashId strategyIdHashId, PricePairPtr pPricePair)
{
	Json::Value back;
	back["strategyIdHashId"] = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId)->key.ToStr();
	back["day"] = Json::Int64(pPricePair->day);
	back["openPrice"] = pPricePair->openPrice;
	back["stopLossPrice"] = pPricePair->stopLossPrice;
	return back;
}

PricePairPtr CJSon_PricePair_Real::Make_PricePair_Item(const Json::Value& itemValue, StrategyIdHashId& strategyIdHashId)
{
	strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(itemValue["strategyIdHashId"].asString());

	PricePairPtr back = std::make_shared<PricePair>();

	back->day = itemValue["day"].asInt64();
	back->openPrice = itemValue["openPrice"].asDouble();
	back->stopLossPrice = itemValue["stopLossPrice"].asDouble();

	return back;

}

Json::Value CJSon_PricePair_Real::MakeValue_PricePairs(const std::map<StrategyIdHashId, PricePairPtr>& pricePairs)
{
	Json::Value back;
	for (const auto& oneItem : pricePairs)
	{
		Json::Value item = MakeValue_PricePair_Item(oneItem.first, oneItem.second);

		back.append(item);
	}
	return back;
}

void CJSon_PricePair_Real::Make_PricePairs(const Json::Value& pricePairsValue, std::map<StrategyIdHashId, PricePairPtr>& pricePairs)
{
	for (unsigned int i = 0; i < pricePairsValue.size(); ++i)
	{
		Json::Value item = pricePairsValue[i];
		StrategyIdHashId strategyIdHashId = 0;
		PricePairPtr pPricePair = Make_PricePair_Item(item, strategyIdHashId);
		pricePairs[strategyIdHashId] = pPricePair;
	}
	return;

}

