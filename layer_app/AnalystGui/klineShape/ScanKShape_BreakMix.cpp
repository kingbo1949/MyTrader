#include "ScanKShape_BreakMix.h"
#include "MakeScanPacket_BreakUTurn.h"
#include <Factory_IBJSon.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
CScanKShape_BreakMix::CScanKShape_BreakMix(QTableView *pTableView)
    :CScanKShape(pTableView)
{
}

KLineShapePtrs CScanKShape_BreakMix::MakeKLineShapes()
{
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);

	//IbContractPtrs contracts = MakeAndGet_JSonContracts()->GetContracts(SelectType::True, std::set<CodeStr>({"NQ"}));

	//Tick_T endTime = CGlobal::GetTimeByStr("20260118 23:00:00") * 1000;

	Tick_T endTime = Get_CurrentTime()->GetCurrentTime_millisecond();

	MakeScanPacketPtr pMakeScanPacket = std::make_shared<CMakeScanPacket_BreakUTurn>();

	// M30线全部
	pMakeScanPacket->Packet(contracts, Time_Type::M30, TopOrBottom::Top, endTime, { KShape::Div});
	pMakeScanPacket->Packet(contracts, Time_Type::M30, TopOrBottom::Bottom, endTime, { KShape::Div});

	// M15线全部
	pMakeScanPacket->Packet(contracts, Time_Type::M15, TopOrBottom::Top, endTime, { KShape::Div});
	pMakeScanPacket->Packet(contracts, Time_Type::M15, TopOrBottom::Bottom, endTime, { KShape::Div});

	return pMakeScanPacket->GetResult();

}

