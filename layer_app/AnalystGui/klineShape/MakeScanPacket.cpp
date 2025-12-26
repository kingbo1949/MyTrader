#include "MakeScanPacket.h"
#include <Factory_IBJSon.h>
#include "KLineShape_Div.h"
CMakeScanPacket::CMakeScanPacket()
{
	MakeAndGet_JSonContracts()->Load_Contracts(m_contracts, SelectType::True);
}
IbContractPtrs CMakeScanPacket::GetContracts(const std::set<CodeStr>& codeIds)
{
	IbContractPtrs ret;
	for (auto contract : m_contracts)
	{
		if (codeIds.find(contract->codeId) != codeIds.end())
		{
			ret.push_back(contract);
		}
	}
	return ret;
}
IbContractPtrs CMakeScanPacket::GetAllContracts()
{
	return m_contracts;
}
IbContractPtrs CMakeScanPacket::GetContracts(SecurityType securityType)
{
	IbContractPtrs ret;

	for (auto contract : m_contracts)
	{
		if (contract->securityType == securityType)
		{
			ret.push_back(contract);
		}
	}

	return ret;
}
void CMakeScanPacket::Packet(const IbContractPtrs& contracts, Time_Type timeType, TopOrBottom topOrBottom, Tick_T endTime, const std::set<KShape>& allowedList)
{
	KShapeInput kShapeInput;
	kShapeInput.contracts = contracts;
	kShapeInput.timeType = timeType;
	kShapeInput.topOrBottom = topOrBottom;
	kShapeInput.endTime = endTime;
	kShapeInput.allowedList = allowedList;
	KLineShapePtr pKLineShape = std::make_shared<CKLineShape_Div>(kShapeInput);
	m_result.push_back(pKLineShape);

}

KLineShapePtrs CMakeScanPacket::GetResult()
{
	return m_result;
}
