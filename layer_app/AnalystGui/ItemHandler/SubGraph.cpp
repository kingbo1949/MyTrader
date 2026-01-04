//
// Created by kingb on 2026/1/4.
//

#include "SubGraph.h"

CSubGraph::CSubGraph(QCustomPlot *parent, SubType subType)
    :CItemsHandler(parent), m_subType(subType)
{
}

void CSubGraph::SubTypeChg(const KlinePlotSuit &klinePlotSuit)
{
    CItemsHandler::SubTypeChg(klinePlotSuit);

    SetKLines(klinePlotSuit);

    // 坐标系可能已经变化 需要调用一下代码
    AxisRangeChgEvent(klinePlotSuit);

}

QCPAxisQPtr CSubGraph::GetX()
{
    return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atBottom);
}

QCPAxisQPtr CSubGraph::GetY()
{
    return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atRight);
}

void CSubGraph::GetWindowsPos(int& beginPos, int& endPos)
{
    QCPRange xRange = GetX()->range();

    beginPos = std::floor(xRange.lower);
    endPos = std::ceil(xRange.upper);
}

bool CSubGraph::IsMyType()
{
    if (m_subType == Get_SubType() && Get_SubVisible() == SubVisible::Show) return true;
    return false;
}
