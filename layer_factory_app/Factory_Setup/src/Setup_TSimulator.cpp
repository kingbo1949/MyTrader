#include "pch.h"
#include "Setup_TSimulator.h"
#include <Factory_IBJSon.h>
#include <Factory_Strategys.h>
#include <Factory_TShareEnv.h>
#include <Factory_QShareEnv.h>
#include <Factory_HashEnv.h>
#include <Factory_UnifyInterface.h>
#include <Factory_QDatabase.h>
#include "Factory_Setup.h"
CSetup_TSimulator::CSetup_TSimulator()
	:CSetup_Real(CurrentTimeType::For_Simulator)
{

}

void CSetup_TSimulator::Init_Factory_Strategys()
{
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	Make_RunTimeStrategys(strategyParams);

	return;

}

void CSetup_TSimulator::RegisterTCallbackObject()
{
	// 模拟交易由行情驱动交易回报，因此不需要注册交易回报的侦听
	return;
}

void CSetup_TSimulator::Init_DaemonObject()
{
	Make_DaemonByTick(SetupType::Simulator);
	return;
}

void CSetup_TSimulator::Init_QCallbackObject()
{
	Make_QCallbackObject(SetupType::Simulator);
	return;
}

void CSetup_TSimulator::Init_Factory_TShareEnv()
{
	MakeAndGet_DbPositionSize();
	MakeAndGet_DbDeal();
	MakeAndGet_DbInactiveOrder();
	MakeAndGet_DbActiveOrder();
	MakeAndGet_DbActiveAction();
	MakeAndGet_DbLastOrder();
	MakeAndGet_DbPricePairs();


	return;

}

void CSetup_TSimulator::Init_Factory_QShareEnv()
{
	Make_CodeIdManager(SetupType::Simulator);
	MakeAndGet_QEnvManager();
}

void CSetup_TSimulator::Init_Factory_StrategyHashEnv()
{
	// 初始化策略哈希属性表
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	Make_StrategyParamEnv(strategyParams);

}

void CSetup_TSimulator::Init_Factory_UnifyInterface()
{
	Make_IBApi(SetupType::Simulator);
	return;
}

void CSetup_TSimulator::RunIBApi()
{
	// 模拟不需要启动api
	return;
}

