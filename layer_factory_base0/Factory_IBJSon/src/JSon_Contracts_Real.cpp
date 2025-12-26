#include "pch.h"
#include "JSon_Contracts_Real.h"
#include <Factory_IBGlobalShare.h>
CJSon_Contracts_Real::CJSon_Contracts_Real()
{

}

CJSon_Contracts_Real::~CJSon_Contracts_Real()
{

}

void CJSon_Contracts_Real::Save_Contracts(const IbContractPtrs& contracts)
{
	Json::Value contractsValue = MakeValue_Contracts(contracts);
	SaveJSonValue(GetFileName(), contractsValue);

}

void CJSon_Contracts_Real::Load_Contracts(IbContractPtrs& contracts, SelectType selectType)
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value contractsValue;
	LoadJSonValue(GetFileName(), contractsValue);

	contracts.clear();
	IbContractPtrs temcontracts = Make_Contracts(contractsValue);
	for (auto onecontract : temcontracts)
	{
		if (selectType == SelectType::All)
		{
			contracts.push_back(onecontract);
		}
		if (selectType == SelectType::True && onecontract->enable)
		{
			contracts.push_back(onecontract);
		}
		if (selectType == SelectType::False && !onecontract->enable)
		{
			contracts.push_back(onecontract);
		}
	}

	//contracts = Make_Contracts(contractsValue);

	return;

}

std::string CJSon_Contracts_Real::GetFileName()
{
	return "./db/contracts.json";
}

Json::Value CJSon_Contracts_Real::MakeValue_Contract(const IbContractPtr& contract)
{
	Json::Value back;
	back["codeId"] = contract->codeId;
	back["localSymbol"] = contract->localSymbol;
	back["securityType"] = CTransToStr::Get_SecurityType(contract->securityType);
	back["exchangePl"] = CTransToStr::Get_ExchangePl(contract->exchangePl) ;
	back["primaryExchangePl"] = CTransToStr::Get_ExchangePl(contract->primaryExchangePl);
	back["currencyID"] = CTransToStr::Get_Money(contract->currencyID);
	back["decDigits"] = contract->decDigits;
	back["minMove"] = contract->minMove;
	back["enable"] = contract->enable;

	return back;

}

IbContractPtr CJSon_Contracts_Real::Make_Contract(const Json::Value& contractValue)
{
	IbContractPtr back = std::make_shared<CIbContract>();

	back->codeId = contractValue["codeId"].asString();
	back->localSymbol = contractValue["localSymbol"].asString();
	back->securityType = CTransToStr::Get_SecurityType(contractValue["securityType"].asString());
	back->exchangePl = CTransToStr::Get_ExchangePl(contractValue["exchangePl"].asString());
	back->primaryExchangePl = CTransToStr::Get_ExchangePl(contractValue["primaryExchangePl"].asString());
	back->currencyID = CTransToStr::Get_Money(contractValue["currencyID"].asString());

	back->decDigits = contractValue["decDigits"].asInt();
	back->minMove = contractValue["minMove"].asDouble();
	back->enable = contractValue["enable"].asBool();

	return back;

}

Json::Value CJSon_Contracts_Real::MakeValue_Contracts(const IbContractPtrs& contracts)
{
	Json::Value back;
	for (IbContractPtrs::const_iterator pos = contracts.begin(); pos != contracts.end(); ++pos)
	{
		if (!(*pos)) continue;


		Json::Value item = MakeValue_Contract(*pos);

		back.append(item);
	}
	return back;

}

IbContractPtrs CJSon_Contracts_Real::Make_Contracts(const Json::Value& contractsValue)
{
	IbContractPtrs back;

	for (unsigned int i = 0; i < contractsValue.size(); ++i)
	{
		Json::Value item = contractsValue[i];
		IbContractPtr contract = Make_Contract(item);
		back.push_back(contract);

	}
	return back;

}
