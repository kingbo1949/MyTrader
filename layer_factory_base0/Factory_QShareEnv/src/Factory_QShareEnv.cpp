#include "pch.h"
#include "Factory_QShareEnv.h"
#include "QEnvManager_Real.h"
#include <Factory_IBJSon.h>


QEnvManagerPtr g_qenvManager = nullptr;
FACTORY_QSHAREENV_API QEnvManagerPtr MakeAndGet_QEnvManager()
{
	if (!g_qenvManager)
	{
		IbContractPtrs contracts;
		MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);

		g_qenvManager = std::make_shared<CQEnvManager_Real>(contracts);
	}
	return g_qenvManager;
}
