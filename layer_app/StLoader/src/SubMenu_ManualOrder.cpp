#include "SubMenu_ManualOrder.h"
#include "QueryPosition.h"
#include <Factory_UnifyInterface.h>

#include <iostream>
#include <string>


void CSubMenu_ManualOrder::ShowMenu()
{
	std::cout <<
		"usage:\n"
		"1: test order: buy to open \n"
		"2: test order: sell to cover \n"
		"3: test order: sell to open \n"
		"4: test order: buy to cover \n"
		"c: test order: cancel order \n"
		"p: my position size \n"
		"x: exit\n";

}

bool CSubMenu_ManualOrder::HandleReceive(const std::string& cmd)
{
	if (cmd == "1")
	{
		Set_Manual_Status(ManualStatus::BuyToOpen);
		return true;
	}
	else if (cmd == "2")
	{
		Set_Manual_Status(ManualStatus::SellToCover);
		return true;
	}
	else if (cmd == "3")
	{
		Set_Manual_Status(ManualStatus::SellToOpen);
		return true;
	}
	else if (cmd == "4")
	{
		Set_Manual_Status(ManualStatus::BuyToCover);
		return true;
	}
	else if (cmd == "c" || cmd == "C")
	{
		Set_Manual_Status(ManualStatus::CancelManul);
		return true;
	}
	else if (cmd == "p" || cmd == "P")
	{
		CQueryPosition queryPs;
		queryPs.GoQuery();
		return true;
	}
	else
	{
		return false;
	}
}
