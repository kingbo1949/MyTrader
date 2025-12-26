#pragma once
#include "AnalystDealDefine.h"
#include "AnalystResultMonth.h"
class CAnalystResultMonth_Real : public CAnalystResultMonth
{
public:
	CAnalystResultMonth_Real(const SimpleMatchPtrs& monthMatches);
	virtual ~CAnalystResultMonth_Real() { ; };


	virtual void				Go() override final;
	virtual std::string			PrintResult() override final;

protected:
	SimpleMatchPtrs				m_monthMatches;
	time_t						m_monthMsSecond = 0;		// 本月1号零点 毫秒
	size_t						m_match_count = 0;			// 本月回合次数
	double						m_profit = 0.0;				// 本月总盈利
	double						m_profitPerMatch = 0.0;		// 本月单笔盈利


};

