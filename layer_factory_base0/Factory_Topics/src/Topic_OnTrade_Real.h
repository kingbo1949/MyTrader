#pragma once
#include "Topic_OnTrade.h"
class CTopic_OnTrade_Real : public CTopic_OnTrade
{
public:
	CTopic_OnTrade_Real() { ; };
	virtual ~CTopic_OnTrade_Real() { ; };

	virtual void					SubscribeTick_OnTrade(PFunc_OnTrade pFunc);
	virtual void					UpdateTick_OnTrade(StrategyIdHashId strategyIdHashId);

protected:
	std::vector<PFunc_OnTrade>	m_ontrade_funcs;

};

