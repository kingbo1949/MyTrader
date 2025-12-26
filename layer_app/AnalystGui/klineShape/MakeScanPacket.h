#pragma once

#include <memory>
#include <base_struc.h>
#include "../klineShape/KLineShape.h"
class CMakeScanPacket
{
public:
	CMakeScanPacket() ;
	virtual ~CMakeScanPacket() { ; };

	IbContractPtrs				GetContracts(const std::set<CodeStr>& codeIds);	// 获取指定品种的合约指针
	IbContractPtrs				GetAllContracts();
	IbContractPtrs				GetContracts(SecurityType securityType);


	void						Packet(const IbContractPtrs& contracts, Time_Type timeType, TopOrBottom	topOrBottom, Tick_T	endTime, const std::set<KShape>& allowedList);

	KLineShapePtrs				GetResult();

protected:
	KLineShapePtrs				m_result;
	IbContractPtrs				m_contracts;

};
typedef std::shared_ptr<CMakeScanPacket> MakeScanPacketPtr;


