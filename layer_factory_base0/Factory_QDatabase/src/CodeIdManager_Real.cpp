#include "pch.h"
#include "CodeIdManager_Real.h"
#include <Factory_HashEnv.h>
#include <Factory_Log.h>
#include <TransToStr.h>
#include <Factory_IBJSon.h>
#include "Factory_QDatabase.h"
CCodeIdManager_Real::CCodeIdManager_Real(SetupType setupType)
{
	Log_Print(LogLevel::Info, fmt::format("call CCodeIdManager_Real {}", CTransToStr::Get_SetupType(setupType).c_str() ) );
	Init(setupType);

	ShowCodes();

}


IbContractPtrs CCodeIdManager_Real::GetCodeId_ForAll()
{
	IbContractPtrs back;

	for (auto onecode : m_codes)
	{
		IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(onecode);
		back.push_back(contract);
	}
	return back;
}

IbContractPtrs CCodeIdManager_Real::GetCodeId_ForSubscribeQ()
{
	// 订阅所有的单品种
	// 因为所有的组合品种元素都必定在单品种中出现，所以
	IbContractPtrs back;

	for (auto onecode : m_codes)
	{
		IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(onecode);

		back.push_back(contract);
	}
	return back;
}

void CCodeIdManager_Real::Init(SetupType setupType)
{
	// 初始化 m_codes
	if (setupType == SetupType::QGenerator)
	{
		// 行情部分 目标文件是contract.json
		IbContractPtrs contracts;
		MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
		for (auto contract : contracts)
		{
			UpdateCode(contract->codeId);
		}
	}
	else
	{
		// 交易部分 目标文件是strategy_param.json
		StrategyParamPtrs strategyParams;
		MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
		for (auto oneparam : strategyParams)
		{
			UpdateCode(oneparam->key.targetCodeId);
		}
	}
}



void CCodeIdManager_Real::UpdateCode(const CodeStr& codeStr)
{
	if (codeStr.find("_") == std::string::npos)
	{
		// 单品种直接插入
		m_codes.insert(codeStr);
	}
	else
	{
		// 混合品种，需要拆分
		//Log_Print(LogLevel::Info, fmt::format("will call GetMixCode, codeId = {}", codeStr.c_str()));
		MixCode mixCode = CChoiceHelper::GetMixCode(codeStr);
		m_codes.insert(mixCode.firstCode);
		m_codes.insert(mixCode.secondCode);

		m_codes.insert(codeStr);
	}


}


void CCodeIdManager_Real::ShowCodes()
{
	Log_Print(LogLevel::Info, fmt::format("{}", GetStr_Codes(m_codes).c_str()) );
}

std::string CCodeIdManager_Real::GetStr_MixCodes(const MixCodes& mixCodes)
{
	std::string back = "";
	for (auto oneMixcode : mixCodes)
	{
		std::string mixCodeStr = Get_CodeIdEnv()->Get_CodeStrByHashId(oneMixcode.myHashId);
		back = back + mixCodeStr + ", ";
	}
	return back;

}

std::string CCodeIdManager_Real::GetStr_Codes(const std::set<CodeStr>& codes)
{
	std::string back = "";
	for (auto onecode : m_codes)
	{
		back = back + onecode + ", ";
	}
	return back;
}
