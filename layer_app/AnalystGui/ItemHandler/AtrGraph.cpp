
#include "AtrGraph.h"
#include <Factory_QDatabase.h>
CAtrGraph::CAtrGraph(QCustomPlot *parent, SubType subType)
    :CSubGraph(parent, subType)
{
    m_atr = m_customPlot->addGraph(GetX(), GetY());
    m_atr->setPen(QColor(255, 109, 0));
    m_atr->setLayer("axes");

}

void CAtrGraph::AxisRangeChgEvent(const KlinePlotSuit &klinePlotSuit)
{
    if (!IsMyType()) return ;

    double high, low;
    int xBeginPos, xEndPos;
    GetWindowsPos(xBeginPos, xEndPos);
    if (xBeginPos < 0) xBeginPos = 0;
    if (xEndPos >= klinePlotSuit.klines.size()) xEndPos = int(klinePlotSuit.klines.size() - 1);

    GetHighLow(klinePlotSuit.klines[xBeginPos]->time, klinePlotSuit.klines[xEndPos]->time + 10, high, low);
    QCPRange yRange(low, high);
    GetY()->setRange(yRange);
    GetY()->scaleRange(GetYAxisFactor(), GetY()->range().center());

    return;

}

void CAtrGraph::SetKLines(const KlinePlotSuit &klinePlotSuit)
{
    if (klinePlotSuit.klines.empty()) return;

    m_atr->data()->clear();
    m_mapAtrValue.clear();

    if (!IsMyType()) return ;

    while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, 0))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto i = 0; i < klinePlotSuit.klines.size(); ++i)
    {
        auto pos = m_mapAtrValue.find(klinePlotSuit.klines[i]->time);
        if (pos == m_mapAtrValue.end()) continue;

        m_atr->addData(i, pos->second->avgAtr);
    }

    // 修改坐标范围
    // 因为有可能是切换品种，而其他元素绘图的范围无法预知，因此不能使用rescaleAxes来自动调整坐标
    // x轴由主图修改，这里不需要修改
    double high, low;
    int xBeginPos, xEndPos;
    GetWindowsPos(xBeginPos, xEndPos);
    if (xBeginPos < 0) xBeginPos = 0;
    if (xEndPos >= int(klinePlotSuit.klines.size()))
    {
        xEndPos = int(klinePlotSuit.klines.size() - 1);
    }

    GetHighLow(klinePlotSuit.klines[xBeginPos]->time, klinePlotSuit.klines[xEndPos]->time + 10, high, low);
    QCPRange yRange(low, high);
    GetY()->setRange(yRange);
    GetY()->scaleRange(GetYAxisFactor(), GetY()->range().center());

}

void CAtrGraph::UpdateKlines(const KlinePlotSuit &klinePlotSuit, const KlineChgCount &klineChgCount)
{
    if (!IsMyType()) return ;

    int addcount = klineChgCount.addcount;
    int chgcount = klineChgCount.chgcount;

    // 本回合数据
    int beginPos = int(klinePlotSuit.klines.size()) - (chgcount + addcount);
    while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, beginPos))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // 删除脏数据
    DelData(beginPos, m_atr);

    for (auto i = beginPos; i < klinePlotSuit.klines.size(); ++i)
    {
        auto pos = m_mapAtrValue.find(klinePlotSuit.klines[i]->time);
        if (pos == m_mapAtrValue.end()) continue;

        m_atr->addData(i, pos->second->avgAtr);
    }

    // 判断新数据是否产生了高低点，而需要改变纵坐标容纳新数据
    double high, low;
    GetHighLow(klinePlotSuit.klines[beginPos]->time, klinePlotSuit.klines.back()->time + 10, high, low);
    QCPRange yRange = GetY()->range();
    if (high > yRange.upper || low < yRange.lower)
    {
        GetY()->scaleRange(GetYAxisFactor(), GetY()->range().center());
    }
    return;

}


IBAtrPtr CAtrGraph::GetAtrValue(Tick_T time)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto pos = m_mapAtrValue.find(time);
    if (pos == m_mapAtrValue.end()) return nullptr;
    return pos->second;
}

bool CAtrGraph::MakeMap(const CodeStr &codeId, Time_Type timetype, const IBKLinePtrs &klines, int beginPos)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (klines.empty()) return false;

    QQuery query;
    query.query_type = QQueryType::BETWEEN_TIME;
    query.time_pair.beginPos = klines[beginPos]->time;
    query.time_pair.endPos = klines.back()->time + 10;

    IBAtrPtrs atrValues = MakeAndGet_QDatabase()->GetAtrs(codeId, timetype, query);
    if (atrValues.empty()) return false;
    if (atrValues.back()->time != klines.back()->time) return false;
    if (atrValues[0]->time != klines[beginPos]->time) return false;


    for (auto value : atrValues)
    {
        m_mapAtrValue[value->time] = value;
    }
    return true;

}

void CAtrGraph::GetHighLow(Tick_T xBeginPos, Tick_T xEndPos, double &high, double &low)
{
    high = -99999999.9;
    low = 99999999.9;

    // 使用 lower_bound 找到键 >= xBeginPos 的第一个元素
    auto it = m_mapAtrValue.lower_bound(xBeginPos);

    // 遍历从 it 到末尾的所有项
    for (; it != m_mapAtrValue.end(); ++it)
    {
        if (it->second->time >= xEndPos) return;

        if (std::isnan(it->second->avgAtr))
        {
            continue;
        }

        if (it->second->avgAtr > high) high = it->second->avgAtr;
        if (it->second->avgAtr < low) low = it->second->avgAtr;
    }
    return;

}
