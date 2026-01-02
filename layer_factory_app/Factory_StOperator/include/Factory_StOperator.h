#pragma once

#include "Factory_StOperator_Export.h"



#include <base_trade.h>
#include "MakePrice.h"
#include "NeedCancel.h"

#include "OpenPrice.h"

	// 取得活跃的挂单
	FACTORY_STOPERATOR_API OneOrderPtr			GetActiveOrder(StrategyIdHashId stHashId, StSubModule stSubModule);

	FACTORY_STOPERATOR_API NeedCancelPtr		MakeAndGet_NeedCancel(const SubModuleParams& stOperator_Input);

	FACTORY_STOPERATOR_API MakePricePtr			MakeAndGet_MakePrice(const SubModuleParams& stOperator_Input);


	FACTORY_STOPERATOR_API OpenPricePtr			MakeAndGet_OpenPrice();



















