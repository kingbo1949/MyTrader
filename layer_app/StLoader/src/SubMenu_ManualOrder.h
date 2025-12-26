#pragma once
#include "SubMenu.h"
class CSubMenu_ManualOrder : public CSubMenu
{
public:
	CSubMenu_ManualOrder() { ; };
	virtual ~CSubMenu_ManualOrder() { ; };

private:
	virtual void		ShowMenu() override final;
	virtual bool		HandleReceive(const std::string& cmd) override final;


};

