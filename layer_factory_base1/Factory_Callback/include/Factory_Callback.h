#pragma once

#ifdef WIN32
#ifdef FACTORY_CALLBACK_EXPORTS
#define FACTORY_CALLBACK_API __declspec(dllexport)
#else
#define FACTORY_CALLBACK_API __declspec(dllimport)
#endif

#else
#define FACTORY_CALLBACK_API
#endif // WIN32


#include "OnRtnQueue.h"
#include "OnRtn.h"

#ifndef WIN32
extern "C"
{
#endif // __cplusplus

	// 
	// 推送委托回报
	FACTORY_CALLBACK_API void					PushOnOrderMsgs(const OnOrderMsg& msg);

	// 推送成交回报
	FACTORY_CALLBACK_API void					PushOnTradeMsgs(const OnTradeMsg& msg);

	FACTORY_CALLBACK_API OnRtnQueuePtr			MakeAndGet_OnRtns();

	FACTORY_CALLBACK_API OnRtnPtr				Make_OnRtn_Order(const OnOrderMsg& msg);
	FACTORY_CALLBACK_API OnRtnPtr				Make_OnRtn_Deal(const OnTradeMsg& msg);

	FACTORY_CALLBACK_API OnRtnPtr				Make_OnRtn_ErrCancel(OneOrderPtr pOriginalOrder);

	





#ifndef WIN32
}
#endif // __cplusplus


