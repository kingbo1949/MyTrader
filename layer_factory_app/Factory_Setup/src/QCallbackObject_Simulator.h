#pragma once
#include "QCallbackObject_Trade.h"
class CQCallbackObject_Simulator : public CQCallbackObject_Trade
{
public:
	CQCallbackObject_Simulator() { ; };
	virtual ~CQCallbackObject_Simulator() { ; };

	virtual void				OnTick(IBTickPtr tick) override final;

protected:
	// 驱动撮合
	void						DriveSimulator();

};

