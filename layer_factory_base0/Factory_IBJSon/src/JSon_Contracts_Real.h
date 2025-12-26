#pragma once
#include "JSon_Contracts.h"
class CJSon_Contracts_Real : public CJSon_Contracts
{
public:
	CJSon_Contracts_Real();
	virtual ~CJSon_Contracts_Real();

	virtual void							Save_Contracts(const IbContractPtrs& contracts) ;

	virtual void							Load_Contracts(IbContractPtrs& contracts, SelectType selectType) override final;


protected:
	std::string								GetFileName();

	Json::Value								MakeValue_Contract(const IbContractPtr& contract);
	IbContractPtr							Make_Contract(const Json::Value& contractValue);

	Json::Value								MakeValue_Contracts(const IbContractPtrs& contracts);
	IbContractPtrs							Make_Contracts(const Json::Value& contractsValue);

};

