#include "pch.h"
#include "Factory_Callback.h"
#include <Factory_QDatabase.h>
#include <Global.h>
#include "OnRtnQueue_Real.h"

#include "OnRtn_Order.h"
#include "OnRtn_Deal.h"
#include "OnRtn_ErrCancel.h"


OnRtnQueuePtr g_pOnRtnQueue = nullptr;
FACTORY_CALLBACK_API void PushOnOrderMsgs(const OnOrderMsg& msg)
{
	OnRtnPtr pOnRtnOrder = Make_OnRtn_Order(msg);
	//MakeAndGet_OnRtns()->AddOne(pOnRtnOrder);

	// 需要立刻更新数据库，以防柜台重复推送
	pOnRtnOrder->Execute();
	return;
}
FACTORY_CALLBACK_API void PushOnTradeMsgs(const OnTradeMsg& msg)
{
	if (CGlobal::IsZero(msg.dealQuantity)) return;

	OnRtnPtr pOnRtnDeal = Make_OnRtn_Deal(msg);
	//MakeAndGet_OnRtns()->AddOne(pOnRtnDeal);

	// 需要立刻持仓数据库，以防客户端重复下单
	pOnRtnDeal->Execute();
	return;
}
FACTORY_CALLBACK_API OnRtnQueuePtr MakeAndGet_OnRtns()
{
	if (!g_pOnRtnQueue)
	{
		g_pOnRtnQueue = std::make_shared<COnRtnQueue_Real>();
	}
	return g_pOnRtnQueue;
}

FACTORY_CALLBACK_API OnRtnPtr Make_OnRtn_Order(const OnOrderMsg& msg)
{
	
	OnRtnPtr pOnRtn = std::make_shared<COnRtn_Order>(msg);
	return pOnRtn;
}


FACTORY_CALLBACK_API OnRtnPtr Make_OnRtn_Deal(const OnTradeMsg& msg)
{
	OnRtnPtr pOnRtn = std::make_shared<COnRtn_Deal>(msg);
	return pOnRtn;
}

FACTORY_CALLBACK_API OnRtnPtr Make_OnRtn_ErrCancel(OneOrderPtr pOriginalOrder)
{
	OnRtnPtr pOnRtn = std::make_shared<COnRtn_ErrCancel>(pOriginalOrder);
	return pOnRtn;
}
