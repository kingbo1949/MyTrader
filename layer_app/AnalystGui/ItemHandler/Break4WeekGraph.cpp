
#include "Break4WeekGraph.h"
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
CBreak4WeekGraph::CBreak4WeekGraph(QCustomPlot *parent)
    :CItemsHandler(parent)
{
    m_graphHigh = m_customPlot->addGraph(GetX(), GetY());
    m_graphHigh->setPen(QPen(MakeAndGet_ColorManager()->GetColor().upper, 2, Qt::DashDotLine));
    //line->setPen(QPen(Qt::blue, 1, Qt::DashLine)); // 蓝色，粗细 1 像素，虚线

    m_graphLow = m_customPlot->addGraph(GetX(), GetY());
    m_graphLow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().lower, 2, Qt::DashDotLine));

    m_graphHigh->setLayer("axes");
    m_graphLow->setLayer("axes");

}

void CBreak4WeekGraph::SetKLines(const KlinePlotSuit &klinePlotSuit)
{
    if (klinePlotSuit.klines.empty()) return;

    m_map.clear();

    while(!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, 0))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ;

    SetKLines(klinePlotSuit.klines, HighOrLow::High);
    SetKLines(klinePlotSuit.klines, HighOrLow::Low);

}

void CBreak4WeekGraph::UpdateKlines(const KlinePlotSuit &klinePlotSuit, const KlineChgCount &klineChgCount)
{
    int addcount = klineChgCount.addcount;
    int chgcount = klineChgCount.chgcount;

    int beginPos = int(klinePlotSuit.klines.size()) - (chgcount + addcount);
    while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, beginPos))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }


    // 本回合数据
    UpdateKlines(klinePlotSuit.klines, HighOrLow::High, addcount, chgcount);
    UpdateKlines(klinePlotSuit.klines, HighOrLow::Low, addcount, chgcount);
}

void CBreak4WeekGraph::NumberKey(int key)
{
    CItemsHandler::NumberKey(key);
    if (key != Qt::Key_4) return;

    m_graphLow->setVisible(!m_graphLow->visible());
    m_graphHigh->setVisible(!m_graphHigh->visible());
    return ;
}

QCPAxisQPtr CBreak4WeekGraph::GetX()
{
    return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CBreak4WeekGraph::GetY()
{
    return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}

bool CBreak4WeekGraph::MakeMap(const CodeStr &codeId, Time_Type timetype, const IBKLinePtrs &klines, int beginPos)
{
    for (int i = beginPos; i < klines.size(); i++)
    {
        const int count = 20;
        IBKLinePtrs subKlines = GetPreviousN(klines, i, count);
        InsertMap(subKlines, klines[i]->time);
    }
    return true;
}

IBKLinePtrs CBreak4WeekGraph::GetPreviousN(const IBKLinePtrs &klines, int pos, int count)
{
    if (pos > klines.size())
    {
        return IBKLinePtrs();
    }
    if (pos < count) return IBKLinePtrs();

    return IBKLinePtrs(
        klines.begin() + (pos - count),
        klines.begin() + pos
    );
}

void CBreak4WeekGraph::InsertMap(const IBKLinePtrs &klines, Tick_T tickTime)
{
    HighLowPair highLowPair;
    if (klines.empty())
    {
        highLowPair.high = std::numeric_limits<double>::quiet_NaN();
        highLowPair.low = std::numeric_limits<double>::quiet_NaN();

    }else
    {
        HighAndLow highAndLow = CHighFrequencyGlobalFunc::MakeHighAndLow(klines, false);
        highLowPair.high = highAndLow.high;
        highLowPair.low = highAndLow.low;
    }
    m_map[tickTime] = highLowPair;
    return ;
}

void CBreak4WeekGraph::SetKLines(const IBKLinePtrs &klines, HighOrLow highOrLow)
{
    QCPGraphQPtr pGraph = GetGraphLine(highOrLow);
    pGraph->data()->clear();

    for (auto i = 0; i < klines.size(); ++i)
    {
        auto pos = m_map.find(klines[i]->time);
        if (pos == m_map.end()) continue;

        if (highOrLow == HighOrLow::High)
        {
            pGraph->addData(i, pos->second.high);
        }else
        {
            pGraph->addData(i, pos->second.low);
        }
    }

}

void CBreak4WeekGraph::UpdateKlines(const IBKLinePtrs &klines, HighOrLow highOrLow, int addcount, int chgcount)
{
    // 本回合数据
    int beginPos = int(klines.size()) - (chgcount + addcount);

    // 删除脏数据
    DelData(beginPos, GetGraphLine(highOrLow));

    for (auto i = beginPos; i < klines.size(); ++i)
    {
        auto pos = m_map.find(klines[i]->time);
        if (pos == m_map.end())
        {
            qDebug() << "can not find:" << CGlobal::GetTickTimeStr(klines[i]->time).c_str();
            continue;
        }

        if (highOrLow == HighOrLow::High)
        {
            GetGraphLine(highOrLow)->addData(i, pos->second.high);
        }else
        {
            GetGraphLine(highOrLow)->addData(i, pos->second.low);
        }

    }

}


QCPGraphQPtr CBreak4WeekGraph::GetGraphLine(HighOrLow highOrLow)
{
    QCPGraphQPtr pGraph;
    if (highOrLow == HighOrLow::High)
    {
        pGraph = m_graphHigh;
    }else
    {
        pGraph = m_graphLow;
    }
    return pGraph;
}
