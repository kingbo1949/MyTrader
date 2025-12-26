#pragma once
#include "ContractEnv.h"
class CContractEnv_Real : public CContractEnv
{
public:
	CContractEnv_Real() ;
	virtual ~CContractEnv_Real() { ; };

	virtual IbContractPtr			GetContract(CodeHashId codeHashId) override final;

	virtual IbContractPtr			GetContract(const CodeStr& codeId) override final;


protected:
	IbContractPtr					m_contracts[CodeHashIdSize];

};

