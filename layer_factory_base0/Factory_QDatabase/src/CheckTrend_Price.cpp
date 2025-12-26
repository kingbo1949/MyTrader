#include "CheckTrend_Price.h"
#include <Factory_IBGlobalShare.h>
#include "Factory_QDatabase.h"
CCheckTrend_Price::CCheckTrend_Price(const CodeStr& codeId, Time_Type timeType, int circle, Tick_T currentTime)
	:CCheckTrend(), m_codeId(codeId), m_timeType(timeType), m_circle(circle), m_currentTime(currentTime)
{

}

bool CCheckTrend_Price::GetTrendType(double price, LongOrShort wantoDo, TrendType& trendType)
{
	m_ma = MakeAndGet_QDatabase()->MakeMa(m_codeId, m_timeType, m_circle, m_currentTime);
	if (m_ma < 0) return false;

	if (price > m_ma)
	{
		if (wantoDo == LongOrShort::LongT)
		{
			trendType = TrendType::ProTrend;
		}
		else
		{
			trendType = TrendType::AntiTrend;
		}
	}
	else
	{
		if (wantoDo == LongOrShort::LongT)
		{
			trendType = TrendType::AntiTrend;
		}
		else
		{
			trendType = TrendType::ProTrend;
		}

	}
	return true;
}

