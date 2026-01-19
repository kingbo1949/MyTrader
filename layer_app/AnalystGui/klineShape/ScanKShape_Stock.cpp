#include "ScanKShape_Stock.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>

#include "MakeScanPacket_Div.h"

CScanKShape_Stock::CScanKShape_Stock(QTableView* pTableView)
	:CScanKShape(pTableView)
{
}

KLineShapePtrs CScanKShape_Stock::MakeKLineShapes()
{
	IbContractPtrs contracts = MakeAndGet_JSonContracts()->GetContracts(SelectType::True, SecurityType::STK);
	Tick_T endTime = Get_CurrentTime()->GetCurrentTime_millisecond();

	MakeScanPacketPtr pMakeScanPacket = std::make_shared<CMakeScanPacket_Div>();

	// 日线全部
	pMakeScanPacket->Packet(contracts, Time_Type::D1, TopOrBottom::Top, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::D1, TopOrBottom::Bottom, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// 小时线全部
	pMakeScanPacket->Packet(contracts, Time_Type::H1, TopOrBottom::Top, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::H1, TopOrBottom::Bottom, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// M30线 不要普通信号
	pMakeScanPacket->Packet(contracts, Time_Type::M30, TopOrBottom::Top, endTime, { KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::M30, TopOrBottom::Bottom, endTime, { KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// M15线 只要DblDivPlus
	pMakeScanPacket->Packet(contracts, Time_Type::M15, TopOrBottom::Top, endTime, { KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::M15, TopOrBottom::Bottom, endTime, { KShape::DblDivPlus, KShape::TrainDiv });

	return pMakeScanPacket->GetResult();

}

