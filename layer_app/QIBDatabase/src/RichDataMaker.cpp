
#include "RichDataMaker.h"
#include "Factory.h"
#include <Factory_Log.h>
#include <Global.h>
CRichDataMaker::CRichDataMaker(const std::string &codeId, ITimeType timeType, const IKLines &klines)
    :m_codeId(codeId), m_timeType(timeType), m_klines(klines)
{
    for (const auto &kline : m_klines)
    {
        IRichValue richValue;
        richValue.time = kline.time;
        richValue.open = kline.open;
        richValue.high = kline.high;
        richValue.low = kline.low;
        richValue.close = kline.close;
        richValue.vol = kline.vol;

        m_mapRick[kline.time] = richValue;
    }
    return;
}

void CRichDataMaker::Go()
{
    FillMa();
    FillMacd();
    FillAtr();
    FillDivType();
    FillPreDay();
    return;

}

IRichValues CRichDataMaker::GetResult()
{
    IRichValues result;
    for (const auto &item : m_mapRick)
    {
        result.push_back(item.second);
    }
    return result;
}

void CRichDataMaker::FillMacd()
{
    if (m_klines.empty()) return;

    IQuery query;
    query.byReqType = 4;
    query.timePair.beginPos = m_klines[0].time;
    query.timePair.endPos = m_klines.back().time;

    IMacdValues values;
    MakeAndGet_Env()->GetDB_Macd()->GetValues(m_codeId, m_timeType, query, values);
    if (values.size() != m_klines.size())
    {
        Log_Print(LogLevel::Critical, fmt::format("klines size {} != macd values size {}", m_klines.size(), values.size()));
    }
    for (const auto &value : values)
    {
        auto pos = m_mapRick.find(value.time);
        if (pos == m_mapRick.end()) continue;

        pos->second.dif = value.dif;
        pos->second.dea = value.dea;
        pos->second.macd = value.macd;
    }
    return;

}

void CRichDataMaker::FillMa()
{
    if (m_klines.empty()) return;

    IQuery query;
    query.byReqType = 4;
    query.timePair.beginPos = m_klines[0].time;
    query.timePair.endPos = m_klines.back().time;

    IAvgValues values;
    MakeAndGet_Env()->GetDB_Ma()->GetValues(m_codeId, m_timeType, query, values);
    if (values.size() != m_klines.size())
    {
        Log_Print(LogLevel::Critical, fmt::format("klines size {} != ma values size {}", m_klines.size(), values.size()));
    }
    for (const auto &value : values)
    {
        auto pos = m_mapRick.find(value.time);
        if (pos == m_mapRick.end()) continue;

        pos->second.ma5 = value.v5;
        pos->second.ma20 = value.v20;
        pos->second.ma60 = value.v60;
        pos->second.ma200 = value.v200;
    }
    return;
}

void CRichDataMaker::FillDivType()
{
    if (m_klines.empty()) return;

    IQuery query;
    query.byReqType = 4;
    query.timePair.beginPos = m_klines[0].time;
    query.timePair.endPos = m_klines.back().time;

    IDivTypeValues values;
    MakeAndGet_Env()->GetDB_DivType()->GetValues(m_codeId, m_timeType, query, values);
    if (values.size() != m_klines.size())
    {
        Log_Print(LogLevel::Critical, fmt::format("klines size {} != divtype values size {}", m_klines.size(), values.size()));
    }
    for (const auto &value : values)
    {
        auto pos = m_mapRick.find(value.time);
        if (pos == m_mapRick.end()) continue;

        pos->second.divType = value.divType;
        pos->second.isUTurn = value.isUTurn;
    }
    return;
}

void CRichDataMaker::FillAtr()
{
    if (m_klines.empty()) return;

    IQuery query;
    query.byReqType = 4;
    query.timePair.beginPos = m_klines[0].time;
    query.timePair.endPos = m_klines.back().time;

    IAtrValues values;
    MakeAndGet_Env()->GetDB_Atr()->GetValues(m_codeId, m_timeType, query, values);
    if (values.size() != m_klines.size())
    {
        Log_Print(LogLevel::Critical, fmt::format("klines size {} != atr values size {}", m_klines.size(), values.size()));
    }
    for (const auto &value : values)
    {
        auto pos = m_mapRick.find(value.time);
        if (pos == m_mapRick.end()) continue;

        pos->second.thisAtr = value.thisAtr;
        pos->second.avgAtr = value.avgAtr;
    }
    return;
}

void CRichDataMaker::FillPreDay()
{
    for (const auto &kline : m_klines)
    {
        IKLine dayKline;
        if (!MakeAndGet_Env()->GetDB_KLine()->GetLastDayKLine(m_codeId, kline.time, dayKline))
        {
            Log_Print(LogLevel::Critical, fmt::format("kline time can not find last day: {}", CGlobal::GetTickTimeStr(kline.time).c_str()));
            continue;;
        }

        auto pos = m_mapRick.find(kline.time);
        if (pos == m_mapRick.end()) continue;

        pos->second.preDayHigh = dayKline.high;
        pos->second.preDayLow = dayKline.low;

    }
}

