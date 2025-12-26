#include "CheckTrend_Crose.h"
#include "Factory_QDatabase.h"
CCheckTrend_Crose::CCheckTrend_Crose(const CodeStr& codeId, Time_Type timeType, int circleFast, int circleSlow, Tick_T currentTime)
	:CCheckTrend(), m_codeId(codeId), m_timeType(timeType), m_circleFast(circleFast), m_circleSlow(circleSlow), m_currentTime(currentTime)
{
}

bool CCheckTrend_Crose::GetTrendType(double price, LongOrShort wantoDo, TrendType& trendType)
{
	m_maFast = MakeAndGet_QDatabase()->MakeMa(m_codeId, m_timeType, m_circleFast, m_currentTime);
	if (m_maFast < 0) return false;
	m_maSlow = MakeAndGet_QDatabase()->MakeMa(m_codeId, m_timeType, m_circleSlow, m_currentTime);
	if (m_maSlow < 0) return false;

	if (m_maFast > m_maSlow)
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
