#include "pch.h"
#include "Factory_IBJSon.h"

#include "JSon_Contracts_Real.h"
#include "JSon_TimeZone_Real.h"
#include "JSon_Broker_Real.h"
#include "JSon_StrategyParam_Real.h"
#include "JSon_TradePoint_Real.h"
#include "JSon_Deal_Real.h"
#include "JSon_PositionSize_Real.h"
#include "JSon_Order_Real.h"
#include "JSon_PricePair_Real.h"
#include "JSon_LastOrder_Real.h"


JSon_ContractsPtr g_Json_Contracts = nullptr;
FACTORY_IBJSON_API JSon_ContractsPtr MakeAndGet_JSonContracts()
{
	if (!g_Json_Contracts)
	{
		g_Json_Contracts = std::make_shared<CJSon_Contracts_Real>();
	}
	return g_Json_Contracts;
}

JSon_TimeZonePtr g_Jsong_TimeZone_QAnalyst = nullptr;
JSon_TimeZonePtr g_Jsong_TimeZone_Simulator = nullptr;
FACTORY_IBJSON_API JSon_TimeZonePtr MakeAndGet_JSonTimeZone(TimeZone_Type timeZone_Type)
{
	if (timeZone_Type == TimeZone_Type::For_QAnalyst)
	{
		if (!g_Jsong_TimeZone_QAnalyst)
		{
			g_Jsong_TimeZone_QAnalyst = std::make_shared<CJSon_TimeZone_Real>(timeZone_Type);
		}
		return g_Jsong_TimeZone_QAnalyst;
	}
	else
	{
		if (!g_Jsong_TimeZone_Simulator)
		{
			g_Jsong_TimeZone_Simulator = std::make_shared<CJSon_TimeZone_Real>(timeZone_Type);
		}
		return g_Jsong_TimeZone_Simulator;

	}

}

JSon_BrokerPtr g_Json_Broker = nullptr;
FACTORY_IBJSON_API JSon_BrokerPtr MakeAndGet_JSonBroker()
{
	if (!g_Json_Broker)
	{
		g_Json_Broker = std::make_shared<CJSon_Broker_Real>();
	}
	return g_Json_Broker;
}

JSon_StrategyParamPtr g_Json_StrategyParam = nullptr;
FACTORY_IBJSON_API JSon_StrategyParamPtr MakeAndGet_JSonStrategyParam()
{
	if (!g_Json_StrategyParam)
	{
		g_Json_StrategyParam = std::make_shared<CJSon_StrategyParam_Real>();
	}
	return g_Json_StrategyParam;
}
JSon_TradePointPtr g_Json_TimePoint = nullptr;
FACTORY_IBJSON_API JSon_TradePointPtr MakeAndGet_JSonTradePoint()
{
	if (!g_Json_TimePoint)
	{
		g_Json_TimePoint = std::make_shared<CJSon_TradePoint_Real>();
	}
	return g_Json_TimePoint;
}

JSon_DealPtr g_Json_Deal = nullptr;
FACTORY_IBJSON_API JSon_DealPtr MakeAndGet_JSonDeal()
{
	if (!g_Json_Deal)
	{
		g_Json_Deal = std::make_shared<CJSon_Deal_Real>();
	}
	return g_Json_Deal;
}

JSon_PositionSizePtr g_Json_PositionSize = nullptr;
FACTORY_IBJSON_API JSon_PositionSizePtr MakeAndGet_JSonPositionSize()
{
	if (!g_Json_PositionSize)
	{
		g_Json_PositionSize = std::make_shared<CJSon_PositionSize_Real>();
	}
	return g_Json_PositionSize;

}


JSon_OrderPtr g_Json_InactiveOrder = nullptr;
FACTORY_IBJSON_API JSon_OrderPtr MakeAndGet_JSonInactiveOrder()
{
	if (!g_Json_InactiveOrder)
	{
		g_Json_InactiveOrder = std::make_shared<CJSon_Order_Real>(ActiveOrInactive::Inactive);
	}
	return g_Json_InactiveOrder;

}

JSon_PricePairPtr g_Json_PricePairs = nullptr;
FACTORY_IBJSON_API JSon_PricePairPtr MakeAndGet_JSonPricePair()
{
	if (!g_Json_PricePairs)
	{
		g_Json_PricePairs = std::make_shared<CJSon_PricePair_Real>();
	}
	return g_Json_PricePairs;
}

JSon_LastOrderPtr g_Json_LastOrders = nullptr;
FACTORY_IBJSON_API JSon_LastOrderPtr MakeAndGet_JSonLastOrder()
{
	if (!g_Json_LastOrders)
	{
		g_Json_LastOrders = std::make_shared<CJSon_LastOrder_Real>();
	}
	return g_Json_LastOrders;
}

