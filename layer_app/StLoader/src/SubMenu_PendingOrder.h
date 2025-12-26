#pragma once
#include "SubMenu.h"
#include <base_trade.h>
class CSubMenu_PendingOrder : public CSubMenu
{
public:
	CSubMenu_PendingOrder() { ; };
	virtual ~CSubMenu_PendingOrder() { ; };

private:
	virtual void		ShowMenu() override final;
	virtual bool		HandleReceive(const std::string& cmd) override final;

	void				SendPendingTestOrder();
	StrategyParamPtr	GetStrategyParamForPendingTestOrder();
	void				CancelPendingTestOrder();

};

