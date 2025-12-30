#pragma once

#include "Factory_UnifyInterface_Export.h"



#include "IBApi.h"
#include <base_struc.h>


	FACTORY_UNIFYINTERFACE_API void				Set_Sys_Status(Thread_Status runOrStop);
	FACTORY_UNIFYINTERFACE_API Thread_Status	Get_Sys_Status();

	FACTORY_UNIFYINTERFACE_API void				Set_Manual_Status(ManualStatus manualStatus);
	FACTORY_UNIFYINTERFACE_API ManualStatus		Get_Manual_Status();

	

	FACTORY_UNIFYINTERFACE_API void				Make_IBApi(SetupType setupType);

	FACTORY_UNIFYINTERFACE_API IBApiPtr			Get_IBApi();

	// 取程序启动时间（秒）
	FACTORY_UNIFYINTERFACE_API Second_T			GetSetupTime();

	// 取程序启动时间（秒）
	FACTORY_UNIFYINTERFACE_API void  			SetSetupTime(Second_T second);




