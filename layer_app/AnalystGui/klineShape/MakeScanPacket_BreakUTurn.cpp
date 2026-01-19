
#include "MakeScanPacket_BreakUTurn.h"
#include "KLineShape_BreakUTurn.h"

void CMakeScanPacket_BreakUTurn::Packet(const IbContractPtrs &contracts, Time_Type timeType, TopOrBottom topOrBottom, Tick_T endTime, const std::set<KShape> &allowedList)
{
    // 这个函数会被反复调用，最后形成一个m_result返回
    KShapeInput kShapeInput;
    kShapeInput.contracts = contracts;
    kShapeInput.timeType = timeType;
    kShapeInput.topOrBottom = topOrBottom;
    kShapeInput.endTime = endTime;
    kShapeInput.allowedList = allowedList;
    KLineShapePtr pKLineShape = std::make_shared<CKLineShape_BreakUTurn>(kShapeInput);
    m_result.push_back(pKLineShape);

}
