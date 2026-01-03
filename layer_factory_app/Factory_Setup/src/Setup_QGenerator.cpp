#include "pch.h"
#include "Setup_QGenerator.h"
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_Topics.h>
#include <Global.h>
#include <Factory_UnifyInterface.h>
#include "Factory_Setup.h"
#include "DaemonByTick_QGenerator.h"

CSetup_QGenerator::CSetup_QGenerator()
	:CSetup_Real(CurrentTimeType::For_Real)
{

}


void CSetup_QGenerator::Init_Factory_Strategys()
{
	// 行情发生器不需要启动交易策略
	return;
}


void CSetup_QGenerator::Init_DaemonObject()
{
	Make_DaemonByTick(SetupType::QGenerator);
	return;
}

void CSetup_QGenerator::Init_QCallbackObject()
{
	Make_QCallbackObject(SetupType::QGenerator);
	return;
}

void CSetup_QGenerator::Init_Factory_TShareEnv()
{
	return;
}

void CSetup_QGenerator::Init_Factory_QShareEnv()
{
	Make_CodeIdManager(SetupType::QGenerator);
	return;
}

void CSetup_QGenerator::Init_Factory_StrategyHashEnv()
{
	return;
}

void CSetup_QGenerator::Init_Factory_UnifyInterface()
{
	Make_IBApi(SetupType::QGenerator);
	return;
}

void CSetup_QGenerator::RunIBApi()
{
	Get_IBApi()->Init();
	Get_IBApi()->Connect();

	// 查询并更新所有品种K线
	IbContractPtrs qContracts;
	MakeAndGet_JSonContracts()->Load_Contracts(qContracts, SelectType::True);
	QueryAndUpdateKline(qContracts);
	MakeAndGet_DaemonByTick_QGenerator()->UpdateDbSecond();

	// 订阅交易所需行情
	IbContractPtrs tContracts = GetStrategyCodeId(CodeIdType::ForSubscribeQ);
	Get_IBApi()->SubscribeQuote(tContracts);
	return;
}

void CSetup_QGenerator::QueryAndUpdateKline(const CodeStr& codeId, Time_Type timeType)
{


	// 查询回溯2天的K线
	int days = 2;

	if (timeType == Time_Type::M30 || timeType == Time_Type::H1 || timeType == Time_Type::D1)
	{ 
		days = 5;	
	}
	time_t endSecond = Get_CurrentTime()->GetCurrentTime_second() + 10;

	bool onlyRegularTime = false;	// 盘前盘后数据都需要查询

	IBKLinePtrs klines = Get_IBApi()->QueryKLine(codeId, timeType, endSecond, days, onlyRegularTime, UseType::QGenerator);

	if (!klines.empty())
	{
		// 开始删除数据 盘中的更新版本在CCmdUpdateDbFromIB，盘中时不删除数据, 在CCmdUpdateDbFromIB盘中也不查询S15周期K线
		TimePair timePair;
		timePair.beginPos = klines[0]->time;
		timePair.endPos = endSecond * 1000;
		MakeAndGet_QDatabase()->RemoveKLines(codeId, timeType, timePair);
	}

	for (auto onekline : klines)
	{
		MakeAndGet_QDatabase()->UpdateKLine(codeId, timeType, onekline);
	}

	// 更新指标
	if (!klines.empty())
	{
		MakeAndGet_QDatabase()->RecountMaFromTimePos(codeId, timeType, klines[0]->time);
		//MakeAndGet_QDatabase()->RecountEmaFromTimePos(codeId, timeType, klines[0]->time);
		MakeAndGet_QDatabase()->RecountMacdFromTimePos(codeId, timeType, klines[0]->time);
		MakeAndGet_QDatabase()->RecountDivTypeFromTimePos(codeId, timeType, klines[0]->time);
		MakeAndGet_QDatabase()->RecountMaFromTimePos(codeId, timeType, klines[0]->time);
	}



	if (klines.empty())
	{
		Log_Print(LogLevel::Info, fmt::format("update to db {} {} size = 0",
			codeId.c_str(),
			CTransToStr::Get_TimeType(timeType).c_str()
		));

	}
	else
	{
		Log_Print(LogLevel::Info, fmt::format("update to db {} {} size = {}, {}->{}",
			codeId.c_str(),
			CTransToStr::Get_TimeType(timeType).c_str(),
			klines.size(),
			CGlobal::GetTickTimeStr(klines[0]->time).c_str(),
			CGlobal::GetTickTimeStr(klines.back()->time).c_str()
		));
	}

	return;
}

void CSetup_QGenerator::QueryAndUpdateKline(IbContractPtrs contracts)
{

	for (auto contract : contracts)
	{
		// 股票不需要15秒线，期货需要
		if (contract->securityType == SecurityType::FUT)
		{
			// 仅期货需要15秒线
			//QueryAndUpdateKline(contract->codeId, Time_Type::S15);
		}
		QueryAndUpdateKline(contract->codeId, Time_Type::M1);
		QueryAndUpdateKline(contract->codeId, Time_Type::M5);
		QueryAndUpdateKline(contract->codeId, Time_Type::M15);
		QueryAndUpdateKline(contract->codeId, Time_Type::M30);
		QueryAndUpdateKline(contract->codeId, Time_Type::H1);
		QueryAndUpdateKline(contract->codeId, Time_Type::D1);
	}
}

void CSetup_QGenerator::RemoveKlines(const CodeStr& codeId, Time_Type timeType, const std::string& beginTimestr, const std::string& endTimeStr)
{
	TimePair timePair;
	timePair.beginPos = CGlobal::GetTimeByStr(beginTimestr) * 1000;
	timePair.endPos = CGlobal::GetTimeByStr(endTimeStr) * 1000;
	MakeAndGet_QDatabase()->RemoveKLines(codeId, timeType, timePair);

}
