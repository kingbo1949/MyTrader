#include "pch.h"
#include "Factory_UnifyInterface.h"
#include "IBApi_IB.h"
#include "IBApi_Simulator.h"
#include <Factory_Log.h>
#include <TransToStr.h>

Thread_Status g_recThead_Status = Thread_Status::Running;
FACTORY_UNIFYINTERFACE_API void Set_Sys_Status(Thread_Status runOrStop)
{
	g_recThead_Status = runOrStop;

}

FACTORY_UNIFYINTERFACE_API  Thread_Status Get_Sys_Status()
{
	return g_recThead_Status;
}
ManualStatus g_manual_status = ManualStatus::Init;
FACTORY_UNIFYINTERFACE_API void Set_Manual_Status(ManualStatus manualStatus)
{
	g_manual_status = manualStatus;
}

FACTORY_UNIFYINTERFACE_API ManualStatus Get_Manual_Status()
{
	return g_manual_status;
}

IBApiPtr g_IbApi = nullptr;
FACTORY_UNIFYINTERFACE_API void Make_IBApi(SetupType setupType)
{
	if (g_IbApi) return;

	if (setupType == SetupType::Simulator)
	{
		g_IbApi = std::make_shared<CIBApi_Simulator>();
	}
	else
	{
		g_IbApi = std::make_shared<CIBApi_IB>();
	}


	return ;
}

FACTORY_UNIFYINTERFACE_API IBApiPtr Get_IBApi()
{
	if (!g_IbApi)
	{
		Log_Print(LogLevel::Err, "g_IbApi is null");
		exit(-1);
	}
	return g_IbApi;
}

Second_T g_setupSecond = 0;
FACTORY_UNIFYINTERFACE_API Second_T GetSetupTime()
{
	return g_setupSecond;
}

FACTORY_UNIFYINTERFACE_API void SetSetupTime(Second_T second)
{
	g_setupSecond = second;
}
