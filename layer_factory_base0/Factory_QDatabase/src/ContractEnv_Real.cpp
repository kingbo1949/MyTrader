#include "pch.h"
#include "ContractEnv_Real.h"
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
CContractEnv_Real::CContractEnv_Real()
{
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	for (auto contract : contracts)
	{
		CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(contract->codeId.c_str() );
		m_contracts[codeHash] = contract;
	}
	return;

}

IbContractPtr CContractEnv_Real::GetContract(CodeHashId codeHashId)
{
	return m_contracts[codeHashId];
}

IbContractPtr CContractEnv_Real::GetContract(const CodeStr& codeId)
{
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str());
	return  GetContract(codeHash);
}
