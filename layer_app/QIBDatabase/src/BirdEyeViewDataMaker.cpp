//
// Created by fan tang on 2026/11/22.
//

#include "BirdEyeViewDataMaker.h"
#include "Factory.h"
#include <Factory_Log.h>
#include <Global.h>

bool CBirdEyeViewDataMaker::Go(const std::string &codeId)
{
    m_codeId = codeId;
    if (!QueryKLine()) return false;
    if (!QueryMa()) return false;
    if (!QueryMacd()) return false;
    if (!QueryDivType()) return false;
    return true;
}

IBirdEyeViewPair CBirdEyeViewDataMaker::GetResult()
{
    return m_birdEyeViewData;
}

bool CBirdEyeViewDataMaker::QueryKLine()
{
    if (!QueryKLine(ITimeType::M1, m_birdEyeViewData.lastOne.birdEyeViewValue_m1.kline, m_birdEyeViewData.thisOne.birdEyeViewValue_m1.kline)) return false;
    if (!QueryKLine(ITimeType::M5, m_birdEyeViewData.lastOne.birdEyeViewValue_m5.kline, m_birdEyeViewData.thisOne.birdEyeViewValue_m5.kline)) return false;
    if (!QueryKLine(ITimeType::M15, m_birdEyeViewData.lastOne.birdEyeViewValue_m15.kline, m_birdEyeViewData.thisOne.birdEyeViewValue_m15.kline)) return false;
    if (!QueryKLine(ITimeType::M30, m_birdEyeViewData.lastOne.birdEyeViewValue_m30.kline, m_birdEyeViewData.thisOne.birdEyeViewValue_m30.kline)) return false;
    if (!QueryKLine(ITimeType::H1, m_birdEyeViewData.lastOne.birdEyeViewValue_h1.kline, m_birdEyeViewData.thisOne.birdEyeViewValue_h1.kline)) return false;
    if (!QueryKLine(ITimeType::D1, m_birdEyeViewData.lastOne.birdEyeViewValue_d1.kline, m_birdEyeViewData.thisOne.birdEyeViewValue_d1.kline)) return false;
    return true;
}

bool CBirdEyeViewDataMaker::QueryKLine(ITimeType timeType, IKLine &lastOne, IKLine &thisOne)
{
    IQuery query;
    query.byReqType = 0;
    query.dwSubscribeNum = 2;

    IKLines klines;
    MakeAndGet_Env()->GetDB_KLine()->GetKLines(m_codeId, timeType, query, klines);
    if (klines.size() != 2)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CBirdEyeViewDataMaker::QueryKLine, klines size = {}", klines.size()));
        return false;
    }
    lastOne = klines[0];
    thisOne = klines[1];
    return true;

}

bool CBirdEyeViewDataMaker::QueryMa()
{
    if (!QueryMa(ITimeType::M1, m_birdEyeViewData.lastOne.birdEyeViewValue_m1.ma, m_birdEyeViewData.thisOne.birdEyeViewValue_m1.ma)) return false;
    if (!QueryMa(ITimeType::M5, m_birdEyeViewData.lastOne.birdEyeViewValue_m5.ma, m_birdEyeViewData.thisOne.birdEyeViewValue_m5.ma)) return false;
    if (!QueryMa(ITimeType::M15, m_birdEyeViewData.lastOne.birdEyeViewValue_m15.ma, m_birdEyeViewData.thisOne.birdEyeViewValue_m15.ma)) return false;
    if (!QueryMa(ITimeType::M30, m_birdEyeViewData.lastOne.birdEyeViewValue_m30.ma, m_birdEyeViewData.thisOne.birdEyeViewValue_m30.ma)) return false;
    if (!QueryMa(ITimeType::H1, m_birdEyeViewData.lastOne.birdEyeViewValue_h1.ma, m_birdEyeViewData.thisOne.birdEyeViewValue_h1.ma)) return false;
    if (!QueryMa(ITimeType::D1, m_birdEyeViewData.lastOne.birdEyeViewValue_d1.ma, m_birdEyeViewData.thisOne.birdEyeViewValue_d1.ma)) return false;
    return true;

}

bool CBirdEyeViewDataMaker::QueryMa(ITimeType timeType, IAvgValue &lastOne, IAvgValue &thisOne)
{
    IQuery query;
    query.byReqType = 0;
    query.dwSubscribeNum = 2;

    IAvgValues mas;
    MakeAndGet_Env()->GetDB_Ma()->GetValues(m_codeId, timeType, query, mas);
    if (mas.size() != 2)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CBirdEyeViewDataMaker::QueryMa, mas size = {}", mas.size()));
        return false;
    }
    lastOne = mas[0];
    thisOne = mas[1];
    return true;
}

bool CBirdEyeViewDataMaker::QueryMacd()
{
    if (!QueryMacd(ITimeType::M1, m_birdEyeViewData.lastOne.birdEyeViewValue_m1.macd, m_birdEyeViewData.thisOne.birdEyeViewValue_m1.macd)) return false;
    if (!QueryMacd(ITimeType::M5, m_birdEyeViewData.lastOne.birdEyeViewValue_m5.macd, m_birdEyeViewData.thisOne.birdEyeViewValue_m5.macd)) return false;
    if (!QueryMacd(ITimeType::M15, m_birdEyeViewData.lastOne.birdEyeViewValue_m15.macd, m_birdEyeViewData.thisOne.birdEyeViewValue_m15.macd)) return false;
    if (!QueryMacd(ITimeType::M30, m_birdEyeViewData.lastOne.birdEyeViewValue_m30.macd, m_birdEyeViewData.thisOne.birdEyeViewValue_m30.macd)) return false;
    if (!QueryMacd(ITimeType::H1, m_birdEyeViewData.lastOne.birdEyeViewValue_h1.macd, m_birdEyeViewData.thisOne.birdEyeViewValue_h1.macd)) return false;
    if (!QueryMacd(ITimeType::D1, m_birdEyeViewData.lastOne.birdEyeViewValue_d1.macd, m_birdEyeViewData.thisOne.birdEyeViewValue_d1.macd)) return false;
    return true;

}

bool CBirdEyeViewDataMaker::QueryMacd(ITimeType timeType, IMacdValue &lastOne, IMacdValue &thisOne)
{
    IQuery query;
    query.byReqType = 0;
    query.dwSubscribeNum = 2;

    IMacdValues macds;
    MakeAndGet_Env()->GetDB_Macd()->GetValues(m_codeId, timeType, query, macds);
    if (macds.size() != 2)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CBirdEyeViewDataMaker::QueryMacd, macds size = {}", macds.size()));
        return false;
    }
    lastOne = macds[0];
    thisOne = macds[1];
    return true;
}

bool CBirdEyeViewDataMaker::QueryDivType()
{
    if (!QueryDivType(ITimeType::M1, m_birdEyeViewData.lastOne.birdEyeViewValue_m1.divTypeValue, m_birdEyeViewData.thisOne.birdEyeViewValue_m1.divTypeValue)) return false;
    if (!QueryDivType(ITimeType::M5, m_birdEyeViewData.lastOne.birdEyeViewValue_m5.divTypeValue, m_birdEyeViewData.thisOne.birdEyeViewValue_m5.divTypeValue)) return false;
    if (!QueryDivType(ITimeType::M15, m_birdEyeViewData.lastOne.birdEyeViewValue_m15.divTypeValue, m_birdEyeViewData.thisOne.birdEyeViewValue_m15.divTypeValue)) return false;
    if (!QueryDivType(ITimeType::M30, m_birdEyeViewData.lastOne.birdEyeViewValue_m30.divTypeValue, m_birdEyeViewData.thisOne.birdEyeViewValue_m30.divTypeValue)) return false;
    if (!QueryDivType(ITimeType::H1, m_birdEyeViewData.lastOne.birdEyeViewValue_h1.divTypeValue, m_birdEyeViewData.thisOne.birdEyeViewValue_h1.divTypeValue)) return false;
    if (!QueryDivType(ITimeType::D1, m_birdEyeViewData.lastOne.birdEyeViewValue_d1.divTypeValue, m_birdEyeViewData.thisOne.birdEyeViewValue_d1.divTypeValue)) return false;
    return true;

}

bool CBirdEyeViewDataMaker::QueryDivType(ITimeType timeType, IDivTypeValue &lastOne, IDivTypeValue &thisOne)
{
    IQuery query;
    query.byReqType = 0;
    query.dwSubscribeNum = 2;

    IDivTypeValues divTypes;
    MakeAndGet_Env()->GetDB_DivType()->GetValues(m_codeId, timeType, query, divTypes);
    if (divTypes.size() != 2)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CBirdEyeViewDataMaker::QueryDivType, divTypes size = {}", divTypes.size()));
        return false;
    }
    lastOne = divTypes[0];
    thisOne = divTypes[1];
    return true;
}
