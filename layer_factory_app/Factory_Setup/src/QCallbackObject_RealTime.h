#pragma once
#include "QCallbackObject_Trade.h"
class CQCallbackObject_RealTime : public CQCallbackObject_Trade
{
public:
	CQCallbackObject_RealTime() { ; };
	virtual ~CQCallbackObject_RealTime() { ; };

	virtual void				OnTick(IBTickPtr tick) override final;

protected:

};

