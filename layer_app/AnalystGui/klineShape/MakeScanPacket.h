#pragma once

#include <memory>
#include <base_struc.h>
#include "KLineShape.h"
class CMakeScanPacket
{
public:
	CMakeScanPacket() { ; };
	virtual ~CMakeScanPacket() { ; };

	virtual void				Packet(const IbContractPtrs& contracts, Time_Type timeType, TopOrBottom	topOrBottom, Tick_T	endTime, const std::set<KShape>& allowedList) = 0;

	KLineShapePtrs				GetResult() { return m_result; };

protected:
	KLineShapePtrs				m_result;


};
typedef std::shared_ptr<CMakeScanPacket> MakeScanPacketPtr;


