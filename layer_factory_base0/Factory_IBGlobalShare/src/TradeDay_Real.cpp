
#include "TradeDay_Real.h"
#include "HighFrequencyGlobalFunc.h"
#include <Factory_Log.h>

#include "Global.h"

CTradeDay_Real::CTradeDay_Real(IBKLinePtrs klines)
    :CTradeDay(klines)
{
    for (auto kline : m_klines)
    {
        if (CHighFrequencyGlobalFunc::IsAfter16(kline->time)) continue;

        Tick_T dayTime = CHighFrequencyGlobalFunc::GetDayMillisec(kline->time);
        m_tradeDay.insert(dayTime);
    }

    if (m_klines.size() != 0 && CHighFrequencyGlobalFunc::IsAfter16(m_klines.back()->time))
    {
        // 最后的数据处于盘后，添加一个伪造的交易日
        Tick_T dayTime = CHighFrequencyGlobalFunc::GetDayMillisec(m_klines.back()->time);
        dayTime += 24 * 60 * 60 * 1000;
        m_tradeDay.insert(dayTime);
    }

    // std::string beginDay = CGlobal::GetTimeStr(*m_tradeDay.begin() / 1000);
    // std::string endDay = CGlobal::GetTimeStr(*std::prev(m_tradeDay.end()) / 1000);
    // Log_Print(LogLevel::Info, fmt::format("tradeday = {} -- {} ", beginDay.c_str(), endDay.c_str()));
    return;

}

bool CTradeDay_Real::GetTradeDay(Tick_T time, Tick_T &tradeDayTime)
{
    Tick_T dayTime = CHighFrequencyGlobalFunc::GetDayMillisec(time);
    if (CHighFrequencyGlobalFunc::IsDaySession(time))
    {
        // 日盘的会计日就是当日
        tradeDayTime = dayTime;
        return true;
    }
    Tick_T dayBeginTime = dayTime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("08:30:00", 0);
    if (time < dayBeginTime)
    {
        // 00:00:00 - 08:30:00 的会计日就是当日
        tradeDayTime = dayTime;
        return true;
    }

    // 找下一个交易日
    auto it = m_tradeDay.upper_bound(dayTime); // 找到严格大于 dayTime 的第一个整数
    if (it == m_tradeDay.end())  return false;

    tradeDayTime = *it;
    return true;


}

IBKLinePtrs CTradeDay_Real::GetTradeDayKLines(Tick_T timeIn)
{
    Tick_T tradeDayTime = 0;
    if (!GetTradeDay(timeIn, tradeDayTime))
    {
        return IBKLinePtrs();
    }

    if (m_tradeDayToKlines.empty())
    {
        FillMap_TradeDayToKlines();
        // PrintMap_TradeDayToKlines();
    }
    std::string tmestr = CGlobal::GetTimeStr(tradeDayTime / 1000);
    auto pos = m_tradeDayToKlines.find(tradeDayTime);
    if (pos == m_tradeDayToKlines.end())
    {
        return IBKLinePtrs();
    }else
    {
        return pos->second;
    }
}

bool CTradeDay_Real::GetLastTradeDay(Tick_T timeIn, Tick_T &lastTradeDayTime)
{
    Tick_T thisDayTime = 0;
    if (!GetTradeDay(timeIn, thisDayTime))
    {
        return false;
    }

    auto it = m_tradeDay.lower_bound(thisDayTime);
    if (it == m_tradeDay.begin())
    {
        return false;
    } else
    {
        lastTradeDayTime = *(--it);
        return true;
    }
}


void CTradeDay_Real::FillMap_TradeDayToKlines()
{
    if (!m_tradeDayToKlines.empty())
    {
        m_tradeDayToKlines.clear();
    }
    for (auto kline : m_klines)
    {
        Tick_T tradeDayTime;
        if (!GetTradeDay(kline->time, tradeDayTime))
        {
            continue;
        }

        auto pos = m_tradeDayToKlines.find(tradeDayTime);
        if (pos == m_tradeDayToKlines.end())
        {
            IBKLinePtrs klines;
            klines.push_back(kline);
            m_tradeDayToKlines[tradeDayTime] = klines;
        }else
        {
            pos->second.push_back(kline);
        }

    }
    return ;
}

void CTradeDay_Real::PrintMap_TradeDayToKlines()
{
    for (auto item : m_tradeDayToKlines)
    {
        std::string day = CGlobal::GetTimeStr(item.first / 1000);
        Log_Print(LogLevel::Info, fmt::format("tradeday = {}, size = {} ", day.c_str(), item.second.size()));
    }
    return ;



}
