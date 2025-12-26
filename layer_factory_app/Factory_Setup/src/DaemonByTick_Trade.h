#pragma once

// 实盘和模拟都是本类作为守护对象

#include "DaemonByTick_Real.h"
class CDaemonByTick_Trade : public CDaemonByTick_Real
{
public:
	CDaemonByTick_Trade();
	virtual ~CDaemonByTick_Trade() { ; };

	virtual void		Execute() override final;

protected:
	time_t				m_lastUpdateSecond_tradePoint;
	time_t				m_lastUpdateSecond_save;


	void				Handle_TradePoint();

	void				Handle_SaveAll();

	void				SavePositionSize();


	void				SaveDeal();

	void				SaveLastOrder();

	void				SavePricePairs();




};

