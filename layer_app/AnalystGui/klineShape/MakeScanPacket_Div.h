#pragma once
#include "MakeScanPacket.h"

class CMakeScanPacket_Div : public CMakeScanPacket
{
public:
    CMakeScanPacket_Div() { ; };
    virtual ~CMakeScanPacket_Div() { ; };

    void				Packet(const IbContractPtrs& contracts, Time_Type timeType, TopOrBottom	topOrBottom, Tick_T	endTime, const std::set<KShape>& allowedList) final;
};


