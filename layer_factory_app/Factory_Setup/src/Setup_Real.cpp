#include "pch.h"
#include "Setup_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_Log.h>
#include <Factory_QDatabase.h>
#include <Factory_QShareEnv.h>
#include <Factory_TShareEnv.h>
#include <Factory_Topics.h>
#include <Factory_UnifyInterface.h>
#include "Factory_Setup.h"

CSetup_Real::CSetup_Real(CurrentTimeType currentType)
	:m_currentType(currentType)
{

}

void CSetup_Real::Open()
{
	// 初始化底层
	Init_Factory_Log();
	Init_Factory_Global();
	Init_Factory_CodeIdHashEnv();
	Init_Factory_StrategyHashEnv();
	Init_Factory_Topics();
	Init_Factory_QDatabase();

	Init_Factory_QShareEnv();
	Init_Factory_TShareEnv();
	Init_Factory_UnifyInterface();
	Init_Factory_Strategys();

	// 初始化守护对象
	Init_DaemonObject();

	// 初始化行情回调对象
	Init_QCallbackObject();

	// 注册行情回调
	RegisterQCallbackObject();

	// 注册交易回调
	RegisterTCallbackObject();

	// 启动远程API
	RunIBApi();

	return;

}

void CSetup_Real::Close()
{
	// 关闭远程api，同时也会导致各种守护线程关闭
	Get_IBApi()->Release();

	//std::this_thread::sleep_for(std::chrono::seconds(2));

	// 关闭行情数据连接
	ReleaseQDatabase();

	std::this_thread::sleep_for(std::chrono::seconds(2));

	// 关闭日志系统
	Log_Close();

	return;

}



void CSetup_Real::RegisterQCallbackObject()
{
	MakeAndGet_Topic_OnTick()->SubscribeTick_OnTick(ReceiveQuote);
	return;
}

void CSetup_Real::Init_Factory_Global()
{
	MakeAndGet_MyThreadPool();
	MakeAndGet_SeriesNoMaker();

	if (m_currentType == CurrentTimeType::For_Real)
	{
		Make_CurrentTime(m_currentType, 0);
	}
	else
	{
		TimeZoneOfCodeIdPtr timeZone = MakeAndGet_JSonTimeZone(TimeZone_Type::For_Simulator)->Load_TimeZone();
		MicroSecond_T beginPos = timeZone->beginPos * 1000 - 1;
		Make_CurrentTime(m_currentType, beginPos);
	}
	

	return;

}

void CSetup_Real::Init_Factory_CodeIdHashEnv()
{
	// 代码哈希属性表
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	Make_CodeIdEnv(CTrans::Get_CodeIds(contracts));


	return;

}

void CSetup_Real::Init_Factory_Log()
{
	Make_Log();

	return;
}

void CSetup_Real::Init_Factory_QDatabase()
{
	// 与行情数据库的连接和ice
	MakeAndGet_QDatabase();

	// 交易时点
	MakeAndGet_TradePointEnv();

	// 代码属性
	MakeAndGet_ContractEnv();

	// 白盘行情的初始化依赖于TradePointEnv和QDatabase
	MakeAndGet_WhiteBoard()->Init();


	return;
}



void CSetup_Real::Init_Factory_Topics()
{
	MakeAndGet_Topic_OnTick();
	MakeAndGet_Topic_OnTrade();
	return;
}


