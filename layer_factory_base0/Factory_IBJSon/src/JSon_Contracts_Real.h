#pragma once
#include "JSon_Contracts.h"
class CJSon_Contracts_Real : public CJSon_Contracts
{
public:
	CJSon_Contracts_Real();
	virtual ~CJSon_Contracts_Real();

	void							Save_Contracts(const IbContractPtrs& contracts) final;

	void							Load_Contracts(IbContractPtrs& contracts, SelectType selectType) final;

	// 获取指定品种的合约指针
	IbContractPtrs					GetContracts(SelectType selectType, const std::set<CodeStr>& codeIds) final;

	// 获取股票或者期货的合约指针
	IbContractPtrs					GetContracts(SelectType selectType, SecurityType securityType) final;



protected:
	IbContractPtrs							m_contracts;
	std::string								GetFileName();

	Json::Value								MakeValue_Contract(const IbContractPtr& contract);
	IbContractPtr							Make_Contract(const Json::Value& contractValue);

	Json::Value								MakeValue_Contracts(const IbContractPtrs& contracts);
	IbContractPtrs							Make_Contracts(const Json::Value& contractsValue);

};

