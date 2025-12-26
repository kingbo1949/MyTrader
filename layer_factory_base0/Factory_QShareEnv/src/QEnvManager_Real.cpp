#include "pch.h"
#include "QEnvManager_Real.h"
#include <Factory_HashEnv.h>
#include "QEnvOneCode_Real.h"
CQEnvManager_Real::CQEnvManager_Real(IbContractPtrs contracts)
{
	for (auto contract : contracts)
	{
		CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(contract->codeId.c_str() );
		m_oneCodeEnvs[codeHash] = std::make_shared<CQEnvOneCode_Real>();
	}

	return;
}

QEnvOneCodePtr CQEnvManager_Real::GetEnvOneCode(CodeHashId codeHashId)
{
	//printf("will get %d %s QEnvOneCodePtr \n", int(codeHashId), Get_CodeIdEnv()->Get_CodeStrByHashId(codeHashId) );
	return m_oneCodeEnvs[codeHashId];
}
