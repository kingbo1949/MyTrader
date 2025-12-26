#include "pch.h"
#include "Topic_OnTick_Real.h"

void CTopic_OnTick_Real::SubscribeTick_OnTick(PFunc_OnTick pFunc)
{
	m_ontick_funcs.push_back(pFunc);
}

void CTopic_OnTick_Real::UpdateTick_OnTick(IBTickPtr tick)
{
	for (const auto& pfunc : m_ontick_funcs)
	{
		pfunc(tick);
	}
	return;

}
