#pragma once
#include "MakeScanPacket.h"


class CMakeScanPacket_BreakUTurn : public CMakeScanPacket
{
public:
    CMakeScanPacket_BreakUTurn() { ; };
    virtual ~CMakeScanPacket_BreakUTurn() { ; };

    void		Packet(const IbContractPtrs& contracts, Time_Type timeType, TopOrBottom	topOrBottom, Tick_T	endTime, const std::set<KShape>& allowedList) final;
};

