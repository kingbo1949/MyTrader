#include "pch.h"
#include "Trans.h"

std::set<CodeStr> CTrans::Get_CodeIds(const IbContractPtrs& contracts)
{
	std::set<CodeStr> back;
	for (const auto& onecontract : contracts)
	{
		back.insert(onecontract->codeId);
	}
	return back;

}
