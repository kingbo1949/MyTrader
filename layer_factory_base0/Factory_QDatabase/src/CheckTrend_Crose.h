#pragma once
#include "CheckTrend.h"
class CCheckTrend_Crose : public CCheckTrend
{
public:
	CCheckTrend_Crose(const CodeStr& codeId, Time_Type timeType, int circleFast, int circleSlow, Tick_T currentTime);
	virtual ~CCheckTrend_Crose() { ; };

	// 查看指定价格的操作是顺势还是逆势
	virtual bool		GetTrendType(double price, LongOrShort wantoDo, TrendType& trendType) override final;

protected:
	CodeStr				m_codeId;
	Time_Type			m_timeType;
	int					m_circleFast;
	int					m_circleSlow;
	Tick_T				m_currentTime;
	double				m_maFast;				// 均线的值
	double				m_maSlow;				// 均线的值


};

