// QTransKLineFormIB.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Factory_IBJSon.h>
#include <Global.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_QDatabase.h>
#include <Factory_HashEnv.h>
#include "KLineConverter_NoMix.h"
#include "SetupEnv.h"
#include "TickConverter.h"
#include "CheckQuote.h"
#include "CFixQuote.h"
#include "FixOneDayQuote.h"
#include "AppFuncs.h"
#include "UpdateKlineFromFile.h"
#include "CleanUpKline.h"
#include "SaveKlineToFile.h"

CSetupEnv setupEnv;
CKLineConverter_NoMix klineConverter_nomix;





void ShowMenu()
{
	std::cout <<
		"usage:\n"
		"a: convert one contract ticks \n"
		"b: update database from files \n"
		"c: clean up klines \n"
		"d: save klines to files \n"
		"0: convert one contract \n"
		"1: query all contracts  \n"
		"2: recount MA for all contracts  \n"
		"3: request one contracts  \n"
		"4: trans all unmix contracts  \n"
		"6: del ticks  \n"
		"7: check quote  \n"
		"8: fix quote  \n"
		"9: fix one day quote  \n"
		"x: exit\n";
	return;
}

// 查询打印所有品种的K线
void QueryAllContracts()
{
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	for (auto onecontract : contracts)
	{
		Log_Print(LogLevel::Info, fmt::format("{} del ticks begin...", onecontract->codeId.c_str()));
		klineConverter_nomix.QueryKLineInDb(onecontract->codeId);
		Log_Print(LogLevel::Info, fmt::format("{} del ticks over", onecontract->codeId.c_str()));
	}
}

// 删除所有品种tick
void DelAllTicks()
{
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	Log_Print(LogLevel::Info, fmt::format("contracts size = {}", contracts.size()));

	for (auto onecontract : contracts)
	{
		Log_Print(LogLevel::Info, fmt::format("{} del ticks begin...", onecontract->codeId.c_str()));
		MakeAndGet_QDatabase()->RemoveAllTick(onecontract->codeId);
		Log_Print(LogLevel::Info, fmt::format("{} del ticks over", onecontract->codeId.c_str()));

	}

}


// 从数据库查询打印单品种的K线
void RequestOneContracts()
{
	time_t beginPos = CGlobal::GetTimeByStr("20230201 00:00:00") * 1000;
	time_t endPos = CGlobal::GetTimeByStr("20230201 16:00:00") * 1000;
	//time_t endPos = Get_CurrentTime()->GetCurrentTime_millisecond();
	klineConverter_nomix.RequestKLineInDb("NQ", Time_Type::M30, beginPos, endPos);

}

// 从ib接口查询所有品种的K线并更新到数据库
void TransAllContracts_NoMix()
{
	int daysForMinute = 60;
	TimePair timePair;
	timePair.beginPos = CGlobal::GetTimeByStr("20191101 00:00:00") * 1000;
	timePair.endPos = Get_CurrentTime()->GetCurrentTime_millisecond();


	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	for (auto onecontract : contracts)
	{

		klineConverter_nomix.ConvertOneKLineFromIBToDb(onecontract->codeId, timePair);
		klineConverter_nomix.QueryKLineInDb(onecontract->codeId);
	}

}

// 从ib接口查询单品种的tick线并更新到数据库
void ConvertOneContractTicks()
{
	std::string targetCodeId = "META";

	TimePair timePair;
	timePair.beginPos = CGlobal::GetTimeByStr("20240328 00:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20240328 15:00:00") * 1000;
	//timePair.endPos = Get_CurrentTime()->GetCurrentTime_millisecond();

	CTickConverter tickConverter;
	tickConverter.ConvertTicksFromIBToDb(targetCodeId, timePair);

	tickConverter.QueryTickInDb(targetCodeId, timePair);

}

// 从ib接口查询单品种的K线并更新到数据库
void ConvertOneContractKLines()
{
	//std::string targetCodeId = "ETHUSDRR";
	std::string targetCodeId = "TSLA";

	TimePair timePair;
	timePair.beginPos = CGlobal::GetTimeByStr("20240101 00:00:00") * 1000;
	//timePair.beginPos = CGlobal::GetTimeByStr("20250702 00:00:00") * 1000;
	//timePair.endPos = CGlobal::GetTimeByStr("20240524 00:00:00") * 1000;
	timePair.endPos = Get_CurrentTime()->GetCurrentTime_millisecond();

	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(targetCodeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	if (contract->securityType == SecurityType::STK)
	{
		// 股票需要删除指定时间段的k线和指标
		CAppFuncs::DelToDb(targetCodeId, Time_Type::S15, timePair);
		CAppFuncs::DelToDb(targetCodeId, Time_Type::M1, timePair);
		CAppFuncs::DelToDb(targetCodeId, Time_Type::M5, timePair);
		CAppFuncs::DelToDb(targetCodeId, Time_Type::M15, timePair);
		CAppFuncs::DelToDb(targetCodeId, Time_Type::M30, timePair);
		CAppFuncs::DelToDb(targetCodeId, Time_Type::H1, timePair);
		CAppFuncs::DelToDb(targetCodeId, Time_Type::D1, timePair);
	}

	// 更新K线和指标
	klineConverter_nomix.ConvertOneKLineFromIBToDb(targetCodeId, timePair);

	klineConverter_nomix.QueryKLineInDb(targetCodeId);

}

// 检查数据库行情数据是否正确
void CheckQuote()
{
	std::string targetCodeId = "ES";

	TimePair timePair;
	timePair.beginPos = CGlobal::GetTimeByStr("20230101 00:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20240523 00:00:00") * 1000;
	//timePair.endPos = Get_CurrentTime()->GetCurrentTime_millisecond();

	// 检查区间为【timePair.beginPos, timePair.endPos】

	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(targetCodeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);

	CCheckQuote checkQuote(contract, timePair);
	//checkQuote.CheckKlines();

	ErrQuotes errQuots = checkQuote.Scan();
	if (errQuots.empty())
	{
		Log_Print(LogLevel::Info, "check quote over, everything is right");
		return;
	}

	for (const auto& oneerr : errQuots)
	{
		CAppFuncs::PrintErr_HighLow_Klines(oneerr.timeType, oneerr.highLow, oneerr.daykline);
	}
}

// 修补行情数据
void FixQuote()
{
	std::string targetCodeId = "NQ";

	TimePair timePair;
	timePair.beginPos = CGlobal::GetTimeByStr("20230305 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230306 18:00:00") * 1000;

	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(targetCodeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);


	CFixQuote fixQuote(contract, timePair);
	fixQuote.Go();
}
// 修补单日行情
void FixOneDayQuote()
{
	std::string targetCodeId = "TSLA";
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(targetCodeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);

	std::vector<time_t> days;

	days.push_back(CGlobal::GetTimeByStr("20240501 00:00:00") * 1000);



	for (const auto& dayTimeMs : days)
	{
		CFixOneDayQuote fixOneDayQuote(contract, dayTimeMs);
		fixOneDayQuote.Go();
	}

}
// 从文件读取K线并更新数据库
void ReadKlineFromFile()
{
	CUpdateKlineFromFile updateKline;
	updateKline.Go();
}

void CleanUpKlines()
{
	CCleanUpKline cleanup;
	cleanup.Go();
}


int main()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); // 设置控制台为 UTF-8, windows的控制台默认是GBK
#endif

	// 初始化底层
	setupEnv.Init();

	// 启动远程API
	setupEnv.RunIBApi();

	ShowMenu();

	std::string cmd;
	do
	{
		std::cout << "==> \n";
		std::getline(std::cin, cmd);
		printf("receive %s \n", cmd.c_str());

		if (cmd == "a" || cmd == "A")
		{
			ConvertOneContractTicks();
		}
		if (cmd == "b" || cmd == "B")
		{
			ReadKlineFromFile();;
		}
		if (cmd == "c" || cmd == "C")
		{
			CleanUpKlines();;
		}
		if (cmd == "d" || cmd == "D")
		{
			CSaveKlineToFile saveKlineToFile;
			saveKlineToFile.Go();
		}
		else if (cmd == "0")
		{
			ConvertOneContractKLines();
		}
		else if (cmd == "2")
		{
			//RecountMaAllContracts();
		}
		else if (cmd == "1")
		{
			QueryAllContracts();
		}
		else if (cmd == "3")
		{
			RequestOneContracts();
		}
		else if (cmd == "4")
		{
			TransAllContracts_NoMix();
		}
		else if (cmd == "6")
		{
			DelAllTicks();
		}
		else if (cmd == "7")
		{
			CheckQuote();
		}
		else if (cmd == "8")
		{
			FixQuote();
		}
		else if (cmd == "9")
		{
			FixOneDayQuote();
		}
		else if (cmd == "x" || cmd == "X")
		{
			;
		}
		else
		{
			std::cout << "unknown command [" << cmd << "]" << std::endl;
			ShowMenu();
		}
	} while (std::cin.good() && cmd != "x" && cmd != "X");

	// 准备退出
	setupEnv.Release();


	return 0;

}


