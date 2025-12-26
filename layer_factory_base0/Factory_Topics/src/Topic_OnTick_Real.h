#pragma once
#include "Topic_OnTick.h"
class CTopic_OnTick_Real : public CTopic_OnTick
{
public:
	CTopic_OnTick_Real() { ; };
	virtual ~CTopic_OnTick_Real() { ; };



	virtual void						SubscribeTick_OnTick(PFunc_OnTick pFunc);

	// tick因为需要使用指针，所以必须是底层白板行情clone()返回的对象
	virtual void						UpdateTick_OnTick(IBTickPtr tick);

protected:
	std::vector<PFunc_OnTick>				m_ontick_funcs;

};

