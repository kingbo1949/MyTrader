#pragma once

#include <set>
#include <base_struc.h>
class CTradeDay
{
public:
    CTradeDay(IBKLinePtrs klines) { m_klines = klines; };
    virtual ~CTradeDay() { ; }

    // 考虑真实业务，得到指定时间的会计交易日
    virtual bool							GetTradeDay(Tick_T time, Tick_T& tradeDayTime) = 0;
    virtual std::set<Tick_T>				GetAllTradeDay()  = 0;

    // timeIn转换为会计日期，然后得到该日期的klines
    // 不是从数据库中取而是从m_klines截取
    virtual IBKLinePtrs                     GetTradeDayKLines(Tick_T timeIn) = 0;

    // timeIn转换为会计日期，然后得到该日期的上一个会计日期，再取其klines
    // 不是从数据库中取而是从m_klines截取
    virtual bool 							GetLastTradeDay(Tick_T time, Tick_T& lastTradeDayTime) = 0;



protected:
    std::set<Tick_T>				        m_tradeDay;
    IBKLinePtrs						        m_klines;


};
typedef std::shared_ptr<CTradeDay> TradeDayPtr;

