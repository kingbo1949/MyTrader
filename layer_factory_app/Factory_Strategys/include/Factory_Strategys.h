#pragma once

#ifdef WIN32

#ifdef FACTORY_STRATEGYS_EXPORTS
#define FACTORY_STRATEGYS_API __declspec(dllexport)
#else
#define FACTORY_STRATEGYS_API __declspec(dllimport)
#endif
#else
#define FACTORY_STRATEGYS_API 
#endif // WIN32




#include <base_trade.h>

#ifndef WIN32
extern "C"
{

#endif // !WIN32

	FACTORY_STRATEGYS_API void					Make_RunTimeStrategys(const StrategyParamPtrs& params);

	// 行情变动的回调函数
	FACTORY_STRATEGYS_API void 					On_QuoteChg_Strategy(const CIBTick& tick, RoundAction openOrCover);

	// 交易变动的回调函数
	FACTORY_STRATEGYS_API void 					On_TradeChg_Strategy(StrategyIdHashId strategyIdHashId);


#ifndef WIN32
}
#endif // !WIN32







