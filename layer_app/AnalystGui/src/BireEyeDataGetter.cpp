//
// Created by fan tang on 2026/11/22.
//

#include "BireEyeDataGetter.h"

BireEyeDataGetterPtr g_BireEyeDataGetter = nullptr;

void CBireEyeDataGetter::Go(const std::string &codeId)
{
    m_codeId = codeId;
    MakeAndGet_QDatabase()->GetBirdEyeViewData(m_codeId, m_lastOne, m_thisOne);
}

IBBirdEyeViewValues CBireEyeDataGetter::GetLastOne()
{
    return m_lastOne;
}

IBBirdEyeViewValues CBireEyeDataGetter::GetThisOne()
{
    return m_thisOne;
}

IBKLinePtrs CBireEyeDataGetter::GetKLines(Time_Type timeType)
{
    IBKLinePtrs klines;
    if (timeType == Time_Type::M1)
    {
        klines.push_back(m_lastOne.value_m1.kline);
        klines.push_back(m_thisOne.value_m1.kline);
    }
    if (timeType == Time_Type::M5)
    {
        klines.push_back(m_lastOne.value_m5.kline);
        klines.push_back(m_thisOne.value_m5.kline);
    }
    if (timeType == Time_Type::M15)
    {
        klines.push_back(m_lastOne.value_m15.kline);
        klines.push_back(m_thisOne.value_m15.kline);
    }
    if (timeType == Time_Type::M30)
    {
        klines.push_back(m_lastOne.value_m30.kline);
        klines.push_back(m_thisOne.value_m30.kline);
    }
    if (timeType == Time_Type::H1)
    {
        klines.push_back(m_lastOne.value_h1.kline);
        klines.push_back(m_thisOne.value_h1.kline);
    }
    if (timeType == Time_Type::D1)
    {
        klines.push_back(m_lastOne.value_d1.kline);
        klines.push_back(m_thisOne.value_d1.kline);
    }

    return klines;

}

BireEyeDataGetterPtr MakeAndGet_BireEyeDataGetter()
{
    if (g_BireEyeDataGetter == nullptr)
    {
        g_BireEyeDataGetter = std::make_shared<CBireEyeDataGetter>();
    }
    return g_BireEyeDataGetter;

}
