#pragma once

// 查看指定价格的操作是顺势还是逆势 价格在均线之上算顺势
#include "CheckTrend.h"
class CCheckTrend_Price : public CCheckTrend
{
public:
	CCheckTrend_Price(const CodeStr& codeId, Time_Type timeType, int circle, Tick_T currentTime);
	virtual ~CCheckTrend_Price() { ; };

	// 查看指定价格的操作是顺势还是逆势
	virtual bool		GetTrendType(double price, LongOrShort wantoDo, TrendType& trendType) override final;

protected:
	CodeStr				m_codeId;
	Time_Type			m_timeType;
	int					m_circle;
	Tick_T				m_currentTime;
	double				m_ma;				// 均线的值



};

