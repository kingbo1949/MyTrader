#pragma once

#include <base_struc.h>
class CContractEnv
{
public:
	CContractEnv() { ; };
	virtual ~CContractEnv() { ; };

	virtual IbContractPtr			GetContract(CodeHashId codeHashId) = 0;

	virtual IbContractPtr			GetContract(const CodeStr& codeId) = 0;

};
typedef std::shared_ptr<CContractEnv> ContractEnvPtr;
