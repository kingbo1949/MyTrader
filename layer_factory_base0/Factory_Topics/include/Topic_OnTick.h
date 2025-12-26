#pragma once



#include "SignalsDefine.h"
// 行情更新与订阅
class CTopic_OnTick
{
public:
	CTopic_OnTick() { ; };
	virtual ~CTopic_OnTick() { ; };

	virtual void						SubscribeTick_OnTick(PFunc_OnTick pFunc) = 0;

	// tick因为需要使用指针，所以必须是底层白板行情clone()返回的对象
	virtual void						UpdateTick_OnTick(IBTickPtr tick) = 0;


};
typedef std::shared_ptr<CTopic_OnTick> Topic_OnTickPtr;



