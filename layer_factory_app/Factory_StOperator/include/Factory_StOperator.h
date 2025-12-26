#pragma once

#ifdef WIN32

#ifdef FACTORY_STOPERATOR_EXPORTS
#define FACTORY_STOPERATOR_API __declspec(dllexport)
#else
#define FACTORY_STOPERATOR_API __declspec(dllimport)
#endif


#else

#define FACTORY_STOPERATOR_API 

#endif // WIN32


#include <base_trade.h>
#include "MakePrice.h"
#include "NeedCancel.h"

#include "OpenPrice.h"
#include <Factory_Calculator.h>
#ifndef WIN32
extern "C"
{
#endif // !WIN32

	// 取得活跃的挂单
	FACTORY_STOPERATOR_API OneOrderPtr			GetActiveOrder(StrategyIdHashId stHashId, StSubModule stSubModule);

	FACTORY_STOPERATOR_API NeedCancelPtr		MakeAndGet_NeedCancel(const SubModuleParams& stOperator_Input);

	FACTORY_STOPERATOR_API MakePricePtr			MakeAndGet_MakePrice(const SubModuleParams& stOperator_Input);


	FACTORY_STOPERATOR_API OpenPricePtr			MakeAndGet_OpenPrice();

	FACTORY_STOPERATOR_API Container_MacdPtr	MakeAndGet_Container_Macd();

	FACTORY_STOPERATOR_API Container_MacdDivPtr MakeAndGet_Container_MacdDiv();

	FACTORY_STOPERATOR_API Container_AveragePtr MakeAndGet_Container_Ma();













#ifndef WIN32
}
#endif // !WIN32








