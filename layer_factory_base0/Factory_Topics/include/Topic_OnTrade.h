#pragma once


#include "SignalsDefine.h"
class CTopic_OnTrade
{
public:
	CTopic_OnTrade() { ; };
	virtual ~CTopic_OnTrade() { ; };

	virtual void					SubscribeTick_OnTrade(PFunc_OnTrade pFunc) = 0;
	virtual void					UpdateTick_OnTrade(StrategyIdHashId strategyIdHashId) = 0 ;


};
typedef std::shared_ptr<CTopic_OnTrade> Topic_OnTradePtr;




