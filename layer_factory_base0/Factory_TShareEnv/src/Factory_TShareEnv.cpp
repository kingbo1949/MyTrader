#include "pch.h"
#include "Factory_TShareEnv.h"

#include "db_ActiveAction_Real.h"
#include "db_Order_Real.h"
#include "db_PositionSize_Real.h"
#include "db_Deal_Real.h"
#include "db_LastOrder_Real.h"
#include "db_PricePair_Real.h"
#include <Factory_IBJSon.h>
#include <Factory_Log.h>

db_ActiveActionPtr g_pDbActiveAction = nullptr;
FACTORY_TSHAREENV_API db_ActiveActionPtr MakeAndGet_DbActiveAction()
{
	if (!g_pDbActiveAction)
	{
		g_pDbActiveAction = std::make_shared<Cdb_ActiveAction_Real>();
	}
	return g_pDbActiveAction;
}

db_OrderPtr g_pDbActiveOrder = nullptr;
FACTORY_TSHAREENV_API db_OrderPtr MakeAndGet_DbActiveOrder()
{
	if (!g_pDbActiveOrder)
	{
		g_pDbActiveOrder = std::make_shared<Cdb_Order_Real>(ActiveOrInactive::Active);
	}
	return g_pDbActiveOrder;
}

db_OrderPtr g_pDbInactiveOrder = nullptr;
FACTORY_TSHAREENV_API db_OrderPtr MakeAndGet_DbInactiveOrder()
{
	if (!g_pDbInactiveOrder)
	{
		g_pDbInactiveOrder = std::make_shared<Cdb_Order_Real>(ActiveOrInactive::Inactive);
	}
	return g_pDbInactiveOrder;

}

db_DealPtr g_pDbDeal = nullptr;
FACTORY_TSHAREENV_API db_DealPtr MakeAndGet_DbDeal()
{
	if (!g_pDbDeal)
	{
		g_pDbDeal = std::make_shared<Cdb_Deal_Real>();
	}
	return g_pDbDeal;
}

db_PositionSizePtr g_pDbPositionSize = nullptr;
FACTORY_TSHAREENV_API db_PositionSizePtr MakeAndGet_DbPositionSize()
{
	if (!g_pDbPositionSize)
	{
		g_pDbPositionSize = std::make_shared<Cdb_PositionSize_Real>();
	}
	return g_pDbPositionSize;
}


db_PricePairPtr g_pDbPricePairs = nullptr;
FACTORY_TSHAREENV_API db_PricePairPtr MakeAndGet_DbPricePairs()
{
	if (!g_pDbPricePairs)
	{
		g_pDbPricePairs = std::make_shared<Cdb_PricePair_Real>();
	}
	return g_pDbPricePairs;
}

db_LastOrderPtr g_pLastOrderTime = nullptr;
FACTORY_TSHAREENV_API db_LastOrderPtr MakeAndGet_DbLastOrder()
{
	if (!g_pLastOrderTime)
	{
		g_pLastOrderTime = std::make_shared<Cdb_LastOrder_Real>();
	}
	return g_pLastOrderTime;
}

FACTORY_TSHAREENV_API void SaveToJson()
{
	PositionSizePtrs positionsizes = MakeAndGet_DbPositionSize()->GetAll();
	MakeAndGet_JSonPositionSize()->Save_PositionSizes(positionsizes);

	OneDealPtrs deals = MakeAndGet_DbDeal()->GetAll();
	MakeAndGet_JSonDeal()->Save_Deals(deals);



}

FACTORY_TSHAREENV_API void MoveOrderFromActiveToInactive(const OrderKey& key, int localOrderNo)
{
	OneOrderPtr porder = MakeAndGet_DbActiveOrder()->RemoveOne(key, localOrderNo);
	if (!porder) return;

	MakeAndGet_DbInactiveOrder()->AddOne(porder);
	return;
}

FACTORY_TSHAREENV_API OneOrderPtr GetOrderFromActiveOrInactive(int localOrderNo)
{
	OneOrderPtr back = MakeAndGet_DbActiveOrder()->GetOne(localOrderNo);
	if (back) return back;

	back = MakeAndGet_DbInactiveOrder()->GetOne(localOrderNo);
	if (!back)
	{
		Log_Print(LogLevel::Warn, fmt::format("can not find order {} anywhere", localOrderNo));
	}
	return back;

}



FACTORY_TSHAREENV_API OneOrderPtrs GetOrdersFromActiveOrInactive(const OrderKey& key)
{
	OneOrderPtrs back;

	OneOrderPtrs activeorders = MakeAndGet_DbActiveOrder()->GetAll(key);
	for (auto oneorder : activeorders)
	{
		back.insert(oneorder);
	}

	OneOrderPtrs inactiveorders = MakeAndGet_DbInactiveOrder()->GetAll(key);
	for (auto oneorder : inactiveorders)
	{
		back.insert(oneorder);
	}
	return back;

}

