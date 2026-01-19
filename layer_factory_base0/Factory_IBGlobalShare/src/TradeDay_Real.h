#pragma once

#include "TradeDay.h"
class CTradeDay_Real : public CTradeDay
{
public:
    CTradeDay_Real(IBKLinePtrs klines);
    ~CTradeDay_Real() override { ; }

    // 考虑真实业务，得到指定时间的会计交易日
    bool							GetTradeDay(Tick_T time, Tick_T& tradeDayTime) final;
    std::set<Tick_T>				GetAllTradeDay() final { return m_tradeDay; } ;

    // timeIn转换为会计日期，然后得到该日期的klines
    // 不是从数据库中取而是从m_klines截取
    IBKLinePtrs                     GetTradeDayKLines(Tick_T timeIn) final;

    // timeIn转换为会计日期，然后得到该日期的上一个会计日期，再取其klines
    // 不是从数据库中取而是从m_klines截取
    bool 							GetLastTradeDay(Tick_T timeIn, Tick_T& lastTradeDayTime) final;


protected:
    std::map<Tick_T, IBKLinePtrs>   m_tradeDayToKlines;     // 交易日对应的KLINS


    // 用m_klines填充m_tradeDayToKlines
    void                            FillMap_TradeDayToKlines();

    // 打印m_tradeDayToKlines
    void                            PrintMap_TradeDayToKlines();

};

