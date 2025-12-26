#include "pch.h"
#include "JSon_Order_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_HashEnv.h>

CJSon_Order_Real::CJSon_Order_Real(ActiveOrInactive activeOrInactive)
	:m_activeOrInactive(activeOrInactive)
{

}

void CJSon_Order_Real::Save_Orders(const OneOrderPtrs& orders)
{
	Json::Value ordersValue = MakeValue_Orders(orders);
	SaveJSonValue(GetFileName(), ordersValue);

}

void CJSon_Order_Real::Load_Orders(OneOrderPtrs& orders)
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value ordersValue;
	LoadJSonValue(GetFileName(), ordersValue);

	orders = Make_Orders(ordersValue);

	return;

}

std::string CJSon_Order_Real::GetFileName()
{
	if (m_activeOrInactive == ActiveOrInactive::Active)
	{
		return  "./db/orders_active.json";
	}
	else
	{
		return  "./db/orders_inactive.json";
	}


}
Json::Value CJSon_Order_Real::MakeValue_OrderKey(const OrderKey& orderkey)
{
	return MakeValue_TradeKey(orderkey);
}

OrderKey CJSon_Order_Real::Make_OrderKey(const Json::Value& orderkeyValue)
{
	return Make_TradeKey(orderkeyValue);
}


Json::Value CJSon_Order_Real::MakeValue_Order(OneOrderPtr& oneorder)
{
	Json::Value back;

	back["orderKey"] = MakeValue_OrderKey(oneorder->orderKey);
	back["codeHashId"] = std::string(Get_CodeIdEnv()->Get_CodeStrByHashId(oneorder->codeHashId));

	back["localOrderNo"] = oneorder->localOrderNo;
	back["tickIndex"] = oneorder->tickIndex;
	back["tickTime"] = Json::Int64(oneorder->tickTime);

	back["openOrCover"] = CTransToStr::Get_RoundAction(oneorder->openOrCover);
	back["buyOrSell"] = CTransToStr::Get_BuyOrSell(oneorder->buyOrSell);
	back["orderStatus"] = CTransToStr::Get_OrderStatus(oneorder->orderStatus);

	back["orderQuantity"] = oneorder->orderQuantity;
	back["orderPriceReal"] = oneorder->orderPriceReal;
	back["orderPriceStrategy"] = oneorder->orderPriceStrategy;
	back["dealedQuantity"] = oneorder->dealedQuantity;
	back["avgDealedPrice"] = oneorder->avgDealedPrice;
	return back;


}

OneOrderPtr CJSon_Order_Real::Make_Order(const Json::Value& oneorderValue)
{
	OneOrderPtr back = std::make_shared<COneOrder>();

	back->orderKey = Make_OrderKey(oneorderValue["orderKey"]);

	back->codeHashId = Get_CodeIdEnv()->Get_CodeId_Hash(oneorderValue["codeHashId"].asString().c_str());

	back->localOrderNo = oneorderValue["localOrderNo"].asInt();
	back->tickIndex = oneorderValue["tickIndex"].asInt();
	back->tickTime = oneorderValue["tickTime"].asInt64();

	back->openOrCover = CTransToStr::Get_RoundAction(oneorderValue["openOrCover"].asString());
	back->buyOrSell = CTransToStr::Get_BuyOrSell(oneorderValue["buyOrSell"].asString());
	back->orderStatus = CTransToStr::Get_OrderStatus(oneorderValue["orderStatus"].asString());

	back->orderQuantity = oneorderValue["orderQuantity"].asDouble();
	back->orderPriceReal = oneorderValue["orderPriceReal"].asDouble();
	back->orderPriceStrategy = oneorderValue["orderPriceStrategy"].asDouble();
	back->dealedQuantity = oneorderValue["dealedQuantity"].asDouble();
	back->avgDealedPrice = oneorderValue["avgDealedPrice"].asDouble();
	return back;
}

Json::Value CJSon_Order_Real::MakeValue_Orders(const OneOrderPtrs& orders)
{
	Json::Value back;
	for (auto oneorder : orders)
	{
		Json::Value item = MakeValue_Order(oneorder);

		back.append(item);
	}
	return back;

}

OneOrderPtrs CJSon_Order_Real::Make_Orders(const Json::Value& ordersValue)
{
	OneOrderPtrs back;

	for (unsigned int i = 0; i < ordersValue.size(); ++i)
	{
		Json::Value item = ordersValue[i];
		OneOrderPtr oneorder = Make_Order(item);
		back.insert(oneorder);

	}
	return back;

}
