#pragma once

#ifdef WIN32

#ifdef FACTORY_TSHAREENV_EXPORTS
#define FACTORY_TSHAREENV_API __declspec(dllexport)
#else
#define FACTORY_TSHAREENV_API __declspec(dllimport)
#endif


#else

#define FACTORY_TSHAREENV_API 

#endif // WIN32


#include "db_ActiveAction.h"
#include "db_Order.h"
#include "db_PositionSize.h"
#include "db_Deal.h"
#include "db_LastOrder.h"
#include "db_PricePair.h"

#ifndef WIN32
extern "C"
{
#endif // !WIN32

	FACTORY_TSHAREENV_API db_ActiveActionPtr				MakeAndGet_DbActiveAction();

	FACTORY_TSHAREENV_API db_OrderPtr						MakeAndGet_DbActiveOrder();

	FACTORY_TSHAREENV_API db_OrderPtr						MakeAndGet_DbInactiveOrder();

	FACTORY_TSHAREENV_API db_DealPtr						MakeAndGet_DbDeal();

	FACTORY_TSHAREENV_API db_PositionSizePtr				MakeAndGet_DbPositionSize();

	FACTORY_TSHAREENV_API db_PricePairPtr					MakeAndGet_DbPricePairs();

	FACTORY_TSHAREENV_API db_LastOrderPtr					MakeAndGet_DbLastOrder();

	// 将需要的信息保存到json文件落地
	FACTORY_TSHAREENV_API void 								SaveToJson();

	FACTORY_TSHAREENV_API void								MoveOrderFromActiveToInactive(const OrderKey& key, int localOrderNo);

	FACTORY_TSHAREENV_API OneOrderPtr						GetOrderFromActiveOrInactive(int localOrderNo);

	// 得到父委托相同的所有子委托，并且指定了品种
	FACTORY_TSHAREENV_API OneOrderPtrs						GetOrdersFromActiveOrInactive(const OrderKey& key);



	

	


#ifndef WIN32
}
#endif // !WIN32







