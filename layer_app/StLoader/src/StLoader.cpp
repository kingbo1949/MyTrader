// StLoader.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <signal.h>
#include <Factory_IBJSon.h>
#include <Factory_Setup.h>
#include <Factory_UnifyInterface.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_HashEnv.h>
#include <Factory_TShareEnv.h>
#include <Factory_Monitor.h>
#include <Log.h>
#include <Global.h>
#include "CmdQGernerator_Simulator.h"
#include "PrintQuoteLastBar.h"
#include "QueryPosition.h"
#include "CompareQuote.h"

#include "SubMenu_ManualOrder.h"
#include "SubMenu_PendingOrder.h"
#include "DownloadData.h"
void sighandler(int signum);

void ShowMenu();

void SendTick();

// 打印数据库行情 所有品种的最后一个bar
void PrintQuoteBar();


int main()
{
	signal(SIGINT, sighandler);

#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); // 设置控制台为 UTF-8, windows的控制台默认是GBK
#endif


	OneBrokerPtr pBroker = MakeAndGet_JSonBroker()->LoadBroker();
	if (pBroker->setupType == SetupType::QGenerator)
	{
		Log_Print(LogLevel::Err, "setupType can not is QGenerator");
		exit(-1);
	}

	Make_Setup(pBroker->setupType);
	Get_Setup()->Open();

	// 设定启动时间
	SetSetupTime(Get_CurrentTime()->GetCurrentTime_second());


	ShowMenu();

	std::string cmd;
	do
	{
		std::cout << "==> \n";
		getline(std::cin, cmd);
		printf("receive %s \n", cmd.c_str());
		if (cmd == "a" || cmd == "A")
		{
			MakeAndGet_DbPricePairs()->PrintAll();
			MakeAndGet_DbLastOrder()->PrintAll();

		}
		if (cmd == "d" || cmd == "D")
		{
			DownloadDataPtr pDownloadData = std::make_shared<CDownloadData>();
			pDownloadData->Go();
		}

		else if (cmd == "m" || cmd == "M")
		{
			SubMenuPtr pSubMenu = std::make_shared<CSubMenu_ManualOrder>();
			pSubMenu->Go();
			ShowMenu();

		}
		else if (cmd == "p" || cmd == "P")
		{
			SubMenuPtr pSubMenu = std::make_shared<CSubMenu_PendingOrder>();
			pSubMenu->Go();
			ShowMenu();
		}
		else if (cmd == "s" || cmd == "S")
		{
			SendTick();
		}
		else if (cmd == "0")
		{
	
			MakeAndGet_Monitor()->Clear();
			
		}
		else if (cmd == "1")
		{
			MakeAndGet_Monitor()->PrintRecentExec();
			
		}
		else if (cmd == "2" )
		{
			// 检查行情数据库的更新情况 防止行情异常
			// 包括:
			//      查询所有合约最后一个tick 
			//      查询最后更新到数据库的tick 
			//      查询最后更新到数据库的tick 
			PrintQuoteBar();
		}
		else if (cmd == "3")
		{
			CQueryPosition queryPs;
			queryPs.GoQuery();
		}
		else if (cmd == "4")
		{
			CCompareQuote comPareQuote(UseType::RealTrader);
			comPareQuote.Go();
		}
		else if (cmd == "x" || cmd == "X")
		{
			// 准备退出
			//Log_Print(LogLevel::Info, "I will be back, baby!");
			Get_Setup()->Close();
		}
		else
		{
			std::cout << "unknown command [" << cmd << "]" << std::endl;
			ShowMenu();
		}
	} while (std::cin.good() && cmd != "x" && cmd != "X");

	while (Get_Sys_Status() == Thread_Status::Running)
	{
		;
	}
	printf("I will be back, baby! \n");
	
	return 0;


}

void sighandler(int signum)
{
	if (signum != SIGINT) return;

	// ctrl-C,关闭程序
	Get_Setup()->Close();

	return;

}
void ShowMenu()
{
	OneBrokerPtr pBroker = MakeAndGet_JSonBroker()->LoadBroker();

	if (pBroker->setupType == SetupType::Simulator)
	{
		std::cout <<
			"usage:\n"
			"0: clear monitor  \n"
			"1: print monitor  \n"
			"a: show all order times  \n"
			"s: send ticks  \n"
			"x: exit\n";

	}
	else
	{
		std::cout <<
			"usage:\n"
			"0: clear monitor  \n"
			"1: print monitor  \n"
			"2: print quote  \n"
			"3: my position size  \n"
			"4: compare quote  \n"			// 比较数据库与IB查询的K线
			"a: show all order times  \n"
			"d: download realtime data  \n"
			"m: test manual order with quote .... \n"
			"p: test pending order without quote.... \n"
			"x: exit\n";

	}


	return;

}
void SendTick()
{
	// 启动一个异步线程，开始发送仿真行情
	CCmdQGernerator_Simulator cmd;
	MakeAndGet_MyThreadPool()->commit(cmd);

}




void PrintQuoteBar()
{
	CPrintQuoteLastBar printer;
	printer.GoPrint();
	return;

}

