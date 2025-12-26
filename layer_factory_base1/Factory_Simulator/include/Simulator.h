#pragma once

#include <base_trade.h>
class CSimulator
{
public:
	CSimulator() { ; };
	virtual ~CSimulator() { ; };

	// 撮合
	virtual void			MeetDeal() = 0;

	// 收到委托
	virtual void			RecOneOrder(OneOrderOfSimulatorPtr pOrder) = 0;

	// 收到撤单
	virtual void			RecOneCancel(int localOrderNo) = 0;



};

typedef std::shared_ptr<CSimulator> SimulatorPtr;
