#pragma once

#include "Factory_Strategys_Export.h"





#include <base_trade.h>


	FACTORY_STRATEGYS_API void					Make_RunTimeStrategys(const StrategyParamPtrs& params);

	// 行情变动的回调函数
	FACTORY_STRATEGYS_API void 					On_QuoteChg_Strategy(const CIBTick& tick, RoundAction openOrCover);

	// 交易变动的回调函数
	FACTORY_STRATEGYS_API void 					On_TradeChg_Strategy(StrategyIdHashId strategyIdHashId);









