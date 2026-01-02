#include "pch.h"
#include "Factory_StOperator.h"

#include <Factory_TShareEnv.h>
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_HashEnv.h>
#include <Factory_QShareEnv.h>
#include <Factory_QDatabase.h>
#include "AppDefine.h"


#include "NeedCancel_Manual.h"
#include "MakePrice_Manual.h"


#include "OpenPrice_Real.h"


FACTORY_STOPERATOR_API	OneOrderPtr GetActiveOrder(StrategyIdHashId stHashId, StSubModule stSubModule)
{
	OrderKey key;
	key.strategyIdHashId = stHashId;
	key.stSubModule = stSubModule;
	OneOrderPtrs temOrders = MakeAndGet_DbActiveOrder()->GetAll(key);
	if (temOrders.empty()) return nullptr;

	OneOrderPtrs orders;
	for (auto oneorder : temOrders)
	{
		orders.insert(oneorder);
	}

	if (orders.size() != 1)
	{
		// 模块中的挂单永远应该只有一个
		Log_Print(LogLevel::Err, fmt::format("active orders.size() != 1 in {}, {}, orders.size() = {}",
			Get_StrategyParamEnv()->Get_StrategyParam(stHashId)->key.ToStr(), CTransToStr::Get_StSubModule(stSubModule).c_str(), orders.size()
		));
		exit(-1);
	}

	OneOrderPtr back = *(orders.begin());
	return back;

}

FACTORY_STOPERATOR_API NeedCancelPtr MakeAndGet_NeedCancel(const SubModuleParams& subModuleParams)
{
	NeedCancelPtr back = nullptr;
	StrategyParamPtr  pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(subModuleParams.strategyIdHashId);

	if (pStrategyParam->key.strategyName == ManualName)
	{
		back = std::make_shared<CNeedCancel_Manual>(subModuleParams);
	}

	if (!back)
	{
		Log_Print(LogLevel::Err, fmt::format("can not make needCancel for {} \n", pStrategyParam->key.strategyName.c_str()));
		exit(1);
	}
	return back;

}

FACTORY_STOPERATOR_API MakePricePtr MakeAndGet_MakePrice(const SubModuleParams& subModuleParams)
{
	MakePricePtr back = nullptr;
	StrategyParamPtr  pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(subModuleParams.strategyIdHashId);

	if (pStrategyParam->key.strategyName == ManualName)
	{
		back = std::make_shared<CMakePrice_Manual>(subModuleParams);
	}

	if (!back)
	{
		Log_Print(LogLevel::Err, fmt::format("can not makeprice for {}", pStrategyParam->key.strategyName.c_str()));
		exit(1);
	}
	return back;

}

OpenPricePtr g_pOpenPrice = nullptr;
FACTORY_STOPERATOR_API OpenPricePtr MakeAndGet_OpenPrice()
{
	if (!g_pOpenPrice)
	{
		g_pOpenPrice = std::make_shared<COpenPrice_Real>();
	}
	return g_pOpenPrice;
}



