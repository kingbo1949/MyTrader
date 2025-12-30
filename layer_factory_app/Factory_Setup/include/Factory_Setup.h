#pragma once

#include "Factory_Setup_Export.h"




#include <base_struc.h>
#include "Setup.h"
#include "DaemonByTick.h"
#include "QCallbackObject.h"

enum class CodeIdType
{
	ForAll,
	ForSubscribeQ
};



	FACTORY_SETUP_API void					Make_DaemonByTick(SetupType setupType);

	FACTORY_SETUP_API DaemonByTickPtr		Get_DaemonByTick();

	FACTORY_SETUP_API void 					Make_QCallbackObject(SetupType setupType);

	FACTORY_SETUP_API QCallbackObjectPtr	Get_QCallbackObject();

	FACTORY_SETUP_API void					Make_Setup(SetupType setupType);

	FACTORY_SETUP_API SetupPtr				Get_Setup();

	// 用于行情注册 接收到tick后做后续处理
	FACTORY_SETUP_API void					ReceiveQuote(IBTickPtr tick);

	// 得到需要交易的品种ID，这些品种会在IB API上订阅
	FACTORY_SETUP_API IbContractPtrs		GetStrategyCodeId(CodeIdType codeIdType);






