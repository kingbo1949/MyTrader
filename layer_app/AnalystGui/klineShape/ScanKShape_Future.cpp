#include "ScanKShape_Future.h"
#include <Factory_IBGlobalShare.h>
CScanKShape_Future::CScanKShape_Future(QTableView* pTableView)
	:CScanKShape(pTableView)
{
}
void CScanKShape_Future::Package(MakeScanPacketPtr pMakeScanPacket)
{
	IbContractPtrs contracts = pMakeScanPacket->GetContracts(SecurityType::FUT);
	Tick_T endTime = Get_CurrentTime()->GetCurrentTime_millisecond();

	// 日线全部
	pMakeScanPacket->Packet(contracts, Time_Type::D1, TopOrBottom::Top, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::D1, TopOrBottom::Bottom, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// 小时线全部
	pMakeScanPacket->Packet(contracts, Time_Type::H1, TopOrBottom::Top, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::H1, TopOrBottom::Bottom, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// M30线全部
	pMakeScanPacket->Packet(contracts, Time_Type::M30, TopOrBottom::Top, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::M30, TopOrBottom::Bottom, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// M15线全部
	pMakeScanPacket->Packet(contracts, Time_Type::M15, TopOrBottom::Top, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::M15, TopOrBottom::Bottom, endTime, { KShape::Div, KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// M5线 不要普通信号
	pMakeScanPacket->Packet(contracts, Time_Type::M5, TopOrBottom::Top, endTime, { KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::M5, TopOrBottom::Bottom, endTime, { KShape::DblDiv, KShape::DblDivPlus, KShape::TrainDiv });

	// M1线 只要DblDivPlus
	pMakeScanPacket->Packet(contracts, Time_Type::M1, TopOrBottom::Top, endTime, { KShape::DblDivPlus, KShape::TrainDiv });
	pMakeScanPacket->Packet(contracts, Time_Type::M1, TopOrBottom::Bottom, endTime, { KShape::DblDivPlus, KShape::TrainDiv });

	return;

}
