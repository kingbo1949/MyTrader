#pragma once

// 这个类负责处理交易日 
#include <base_struc.h>
#include <set>

class CTradeDay
{
public:
	CTradeDay(IBKLinePtrs klines);
	virtual ~CTradeDay() { ; }

	// 考虑真实业务，得到指定时间的会计交易日
	bool							GetTradeDay(Tick_T time, Tick_T& tradeDayTime);
	std::set<Tick_T>				GetAllTradeDay() { return m_tradeDay; };

protected:
	IBKLinePtrs						m_klines;
	std::set<Tick_T>				m_tradeDay;

	// 不考虑真实业务，仅按m_tradeDay计算给定时间的下一个交易日
	bool 							GetNextTradeDay(Tick_T time, Tick_T& nextTradeDayTime);

};
typedef std::shared_ptr<CTradeDay> TradeDayPtr;
