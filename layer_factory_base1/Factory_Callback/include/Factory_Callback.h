#pragma once
#include "Factory_Callback_Export.h"



#include "OnRtnQueue.h"
#include "OnRtn.h"


	// 
	// 推送委托回报
	FACTORY_CALLBACK_API void					PushOnOrderMsgs(const OnOrderMsg& msg);

	// 推送成交回报
	FACTORY_CALLBACK_API void					PushOnTradeMsgs(const OnTradeMsg& msg);

	FACTORY_CALLBACK_API OnRtnQueuePtr			MakeAndGet_OnRtns();

	FACTORY_CALLBACK_API OnRtnPtr				Make_OnRtn_Order(const OnOrderMsg& msg);
	FACTORY_CALLBACK_API OnRtnPtr				Make_OnRtn_Deal(const OnTradeMsg& msg);

	FACTORY_CALLBACK_API OnRtnPtr				Make_OnRtn_ErrCancel(OneOrderPtr pOriginalOrder);

	






