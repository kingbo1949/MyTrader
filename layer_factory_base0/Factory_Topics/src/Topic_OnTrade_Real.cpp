#include "pch.h"
#include "Topic_OnTrade_Real.h"

void CTopic_OnTrade_Real::SubscribeTick_OnTrade(PFunc_OnTrade pFunc)
{
	m_ontrade_funcs.push_back(pFunc);
}

void CTopic_OnTrade_Real::UpdateTick_OnTrade(StrategyIdHashId strategyIdHashId)
{
	for (const auto& pfunc : m_ontrade_funcs)
	{
		pfunc(strategyIdHashId);
	}
	return;

}
