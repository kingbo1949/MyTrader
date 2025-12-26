#pragma once
#include "JSon.h"
#include <base_trade.h>
class CJSon_StrategyParam : public CJSon
{
public:
	CJSon_StrategyParam() { ; };
	virtual ~CJSon_StrategyParam() { ; };

	virtual void			Save_StrategyParams(const StrategyParamPtrs& strategyParams) = 0;

	virtual void			Load_StrategyParams(StrategyParamPtrs& strategyParams) = 0;

};
typedef std::shared_ptr<CJSon_StrategyParam> JSon_StrategyParamPtr;

