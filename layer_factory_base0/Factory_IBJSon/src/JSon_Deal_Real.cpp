#include "pch.h"
#include "JSon_Deal_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_HashEnv.h>

void CJSon_Deal_Real::Save_Deals(const OneDealPtrs& deals)
{
	Json::Value dealsValue = MakeValue_Deals(deals);
	SaveJSonValue(GetFileName(), dealsValue);

}

void CJSon_Deal_Real::Load_Deals(OneDealPtrs& deals)
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value dealsValue;
	LoadJSonValue(GetFileName(), dealsValue);

	deals = Make_Deals(dealsValue);

	return;

}

std::string CJSon_Deal_Real::GetFileName()
{
	return  "./db/deals.json";
}

Json::Value CJSon_Deal_Real::MakeValue_DealKey(const DealKey& dealkey)
{
	return MakeValue_TradeKey(dealkey);
}

DealKey CJSon_Deal_Real::Make_DealKey(const Json::Value& dealkeyValue)
{
	return Make_TradeKey(dealkeyValue);
}
Json::Value CJSon_Deal_Real::MakeValue_Deal(OneDealPtr& onedeal)
{
	Json::Value back;

	back["key"] = MakeValue_DealKey(onedeal->key);
	back["codeHashId"] = std::string(Get_CodeIdEnv()->Get_CodeStrByHashId(onedeal->codeHashId));

	back["dealNo"] = onedeal->dealNo;
	back["localOrderNo"] = onedeal->localOrderNo;

	back["openOrCover"] = CTransToStr::Get_RoundAction(onedeal->openOrCover);
	back["buyOrSell"] = CTransToStr::Get_BuyOrSell(onedeal->buyOrSell);

	back["dealQuantity"] = onedeal->dealQuantity;
	back["dealPrice"] = onedeal->dealPrice;

	back["dealIndex"] = onedeal->dealIndex;
	back["dealTickTime"] = Json::Int64(onedeal->dealTickTime);
	back["fee"] = onedeal->fee;
	return back;

}

OneDealPtr CJSon_Deal_Real::Make_Deal(const Json::Value& onedealValue)
{
	OneDealPtr back = std::make_shared<COneDeal>();

	back->key = Make_DealKey(onedealValue["key"]);
	back->codeHashId = Get_CodeIdEnv()->Get_CodeId_Hash(onedealValue["codeHashId"].asString().c_str());

	back->dealNo = onedealValue["dealNo"].asInt();
	back->localOrderNo = onedealValue["localOrderNo"].asInt();

	back->openOrCover = CTransToStr::Get_RoundAction(onedealValue["openOrCover"].asString());
	back->buyOrSell = CTransToStr::Get_BuyOrSell(onedealValue["buyOrSell"].asString());

	back->dealQuantity = onedealValue["dealQuantity"].asDouble();
	back->dealPrice = onedealValue["dealPrice"].asDouble();

	back->dealIndex = onedealValue["dealIndex"].asInt();
	back->dealTickTime = onedealValue["dealTickTime"].asInt64();
	back->fee = onedealValue["fee"].asDouble();
	return back;

}

Json::Value CJSon_Deal_Real::MakeValue_Deals(const OneDealPtrs& deals)
{
	Json::Value back;
	for (auto onedeal : deals)
	{
		Json::Value item = MakeValue_Deal(onedeal);

		back.append(item);
	}
	return back;

}

OneDealPtrs CJSon_Deal_Real::Make_Deals(const Json::Value& dealsValue)
{
	OneDealPtrs back;

	for (unsigned int i = 0; i < dealsValue.size(); ++i)
	{
		Json::Value item = dealsValue[i];
		OneDealPtr onedeal = Make_Deal(item);
		back.insert(onedeal);

	}
	return back;

}
