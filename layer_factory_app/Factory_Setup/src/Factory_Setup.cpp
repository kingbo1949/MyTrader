#include "pch.h"
#include "Factory_Setup.h"
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <Factory_Log.h>
#include <Factory_HashEnv.h>
#include <Factory_Callback.h>
#include <Factory_IBJSon.h>
#include "Setup_QGenerator.h"
#include "Setup_TRunTime.h"
#include "Setup_TSimulator.h"

#include "DaemonByTick_QGenerator.h"
#include "DaemonByTick_Trade.h"

#include "QCallbackObject_QGenerator.h"
#include "QCallbackObject_RealTime.h"
#include "QCallbackObject_Simulator.h"



DaemonByTickPtr g_pDaemonByTick = nullptr;
FACTORY_SETUP_API void Make_DaemonByTick(SetupType setupType)
{
	if (setupType == SetupType::QGenerator)
	{
		g_pDaemonByTick = MakeAndGet_DaemonByTick_QGenerator();
	}
	if (setupType == SetupType::RealTrader || setupType == SetupType::Simulator)
	{
		g_pDaemonByTick = std::make_shared<CDaemonByTick_Trade>();
	}
}

FACTORY_SETUP_API DaemonByTickPtr Get_DaemonByTick()
{
	if (!g_pDaemonByTick)
	{
		Log_Print(LogLevel::Err, "g_pDaemonByTick is null");
		exit(-1);
	}
	return g_pDaemonByTick;
}

QCallbackObjectPtr g_pQCallbackObject = nullptr;
FACTORY_SETUP_API void Make_QCallbackObject(SetupType setupType)
{
	if (g_pQCallbackObject) return;

	if (setupType == SetupType::QGenerator)
	{
		g_pQCallbackObject = std::make_shared<CQCallbackObject_QGenerator>();
	}
	if (setupType == SetupType::RealTrader)
	{
		g_pQCallbackObject = std::make_shared<CQCallbackObject_RealTime>();
	}
	if (setupType == SetupType::Simulator)
	{
		g_pQCallbackObject = std::make_shared<CQCallbackObject_Simulator>();
	}



}

FACTORY_SETUP_API QCallbackObjectPtr Get_QCallbackObject()
{
	if (!g_pQCallbackObject)
	{
		Log_Print(LogLevel::Err, "g_pQCallbackObject is null");
		exit(-1);
	}
	return g_pQCallbackObject;
}

SetupPtr g_setup = nullptr;
FACTORY_SETUP_API void Make_Setup(SetupType setupType)
{
	if (g_setup) return;

	if (setupType == SetupType::QGenerator)
	{
		g_setup = std::make_shared<CSetup_QGenerator>();
	}
	if (setupType == SetupType::RealTrader)
	{
		g_setup = std::make_shared<CSetup_TRunTime>();
	}
	if (setupType == SetupType::Simulator)
	{
		g_setup = std::make_shared<CSetup_TSimulator>();
	}
	return;
}

FACTORY_SETUP_API SetupPtr Get_Setup()
{
	return g_setup;
}

FACTORY_SETUP_API void ReceiveQuote(IBTickPtr tick)
{
	Get_QCallbackObject()->OnTick(tick);
}

FACTORY_SETUP_API IbContractPtrs GetStrategyCodeId(CodeIdType codeIdType)
{
	if (codeIdType == CodeIdType::ForAll)
	{
		return Get_CodeIdManager()->GetCodeId_ForAll();
	}
	else
	{
		return Get_CodeIdManager()->GetCodeId_ForSubscribeQ();
	}
}


