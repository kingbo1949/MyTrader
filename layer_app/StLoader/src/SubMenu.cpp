#include "SubMenu.h"

#include <iostream>
#include <string>


void CSubMenu::Go()
{
	ShowMenu();

	std::string cmd;
	do
	{
		std::cout << "==> \n";
		getline(std::cin, cmd);
		printf("receive %s \n", cmd.c_str());

		if (cmd == "x" || cmd == "X")
		{
			// 退出子菜单
			break;;
		}
		if (!HandleReceive(cmd))
		{
			std::cout << "unknown command [" << cmd << "]" << std::endl;
			ShowMenu();
		}
	} while (std::cin.good() && cmd != "x" && cmd != "X");

	return;

}
