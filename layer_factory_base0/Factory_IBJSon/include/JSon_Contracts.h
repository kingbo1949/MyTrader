#pragma once
#include "JSon.h"
#include <base_struc.h>
class CJSon_Contracts : public CJSon
{
public:
	CJSon_Contracts() { ; };
	virtual ~CJSon_Contracts() { ; };

	virtual void			Save_Contracts(const IbContractPtrs& contracts) = 0;

	virtual void			Load_Contracts(IbContractPtrs& contracts, SelectType selectType) = 0;

	// 获取指定品种的合约指针
	virtual IbContractPtrs	GetContracts(SelectType selectType, const std::set<CodeStr>& codeIds) = 0;

	// 获取股票或者期货的合约指针
	virtual IbContractPtrs	GetContracts(SelectType selectType, SecurityType securityType) = 0;


};
typedef std::shared_ptr<CJSon_Contracts> JSon_ContractsPtr;
