#include "stdafx.h"
#include "TradeDay.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
CTradeDay::CTradeDay(IBKLinePtrs klines)
	:m_klines(klines)
{
	for (auto kline : m_klines)
	{
		//std::string str = CGlobal::GetTickTimeStr(kline->time);
		//Tick_T beginTime = CGlobal::GetTickTimeByStr("2023")
		//if ()
		if (!CHighFrequencyGlobalFunc::IsDaySession(kline->time)) continue;

		Tick_T dayTime = CHighFrequencyGlobalFunc::GetDayMillisec(kline->time);
		m_tradeDay.insert(dayTime);
	}
	return;

}

bool CTradeDay::GetTradeDay(Tick_T time, Tick_T& tradeDayTime)
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
	Tick_T nextTradeDay = 0;
	if (GetNextTradeDay(time, nextTradeDay))
	{
		tradeDayTime = nextTradeDay;
		return true;
	}
	return false;
}

bool CTradeDay::GetNextTradeDay(Tick_T time, Tick_T& nextTradeDayTime)
{
	Tick_T thisDayTime = CHighFrequencyGlobalFunc::GetDayMillisec(time);

	auto it = m_tradeDay.upper_bound(thisDayTime); // 找到严格大于 thisDayTime 的第一个整数
	if (it == m_tradeDay.end())  return false;

	nextTradeDayTime = *it;
	return true;
}
