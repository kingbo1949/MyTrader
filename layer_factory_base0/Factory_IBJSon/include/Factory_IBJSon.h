#pragma once

#ifdef WIN32

#ifdef FACTORY_IBJSON_EXPORTS
#define FACTORY_IBJSON_API __declspec(dllexport)
#else
#define FACTORY_IBJSON_API __declspec(dllimport)
#endif


#else

#define FACTORY_IBJSON_API 

#endif // WIN32


#include "JSon_Contracts.h"
#include "JSon_TimeZone.h"
#include "JSon_Broker.h"
#include "JSon_StrategyParam.h"
#include "JSon_TradePoint.h"
#include "JSon_Deal.h"
#include "JSon_PositionSize.h"
#include "JSon_Order.h"
#include "JSon_LastOrder.h"
#include "JSon_PricePair.h"

#ifndef WIN32
extern "C"
{

#endif // !WIN32


	FACTORY_IBJSON_API JSon_ContractsPtr				MakeAndGet_JSonContracts();

	FACTORY_IBJSON_API JSon_TimeZonePtr					MakeAndGet_JSonTimeZone(TimeZone_Type timeZone_Type);

	FACTORY_IBJSON_API JSon_BrokerPtr					MakeAndGet_JSonBroker();

	FACTORY_IBJSON_API JSon_StrategyParamPtr			MakeAndGet_JSonStrategyParam();

	FACTORY_IBJSON_API JSon_TradePointPtr				MakeAndGet_JSonTradePoint();

	FACTORY_IBJSON_API JSon_DealPtr						MakeAndGet_JSonDeal();

	FACTORY_IBJSON_API JSon_PositionSizePtr				MakeAndGet_JSonPositionSize();

	FACTORY_IBJSON_API JSon_OrderPtr					MakeAndGet_JSonInactiveOrder();

	FACTORY_IBJSON_API JSon_PricePairPtr				MakeAndGet_JSonPricePair();

	FACTORY_IBJSON_API JSon_LastOrderPtr				MakeAndGet_JSonLastOrder();
	

#ifndef WIN32
}
#endif // !WIN32





