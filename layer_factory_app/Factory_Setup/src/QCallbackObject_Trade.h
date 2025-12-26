#pragma once
#include "QCallbackObject.h"
#include <base_trade.h>
class CQCallbackObject_Trade : 	public CQCallbackObject
{
public:
	CQCallbackObject_Trade() { ; };
	virtual ~CQCallbackObject_Trade() { ; };

	virtual void				OnTick(IBTickPtr tick) override { ; };

protected:
	// 更新行情环境
	void				UpdateQShareEnv(IBTickPtr tick);

	// 驱动策略组
	void				DriveStategys(IBTickPtr tick, RoundAction openOrCover);

	// 清理COnRtnQueue中的交易回报
	void				CleanTradeRtnQueue();

	//  撮合模拟成交
	void				DriveMeetDeal();

};

