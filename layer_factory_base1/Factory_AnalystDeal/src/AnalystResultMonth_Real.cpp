#include "pch.h"
#include "AnalystResultMonth_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
//#include <Factory_Log.h>
CAnalystResultMonth_Real::CAnalystResultMonth_Real(const SimpleMatchPtrs& monthMatches)
{
	copy(monthMatches.begin(), monthMatches.end(), back_inserter(m_monthMatches));
}

void CAnalystResultMonth_Real::Go()
{
	if (m_monthMatches.size() == 0) return;
	m_monthMsSecond = CGetRecordNo::GetRecordNo_Month(m_monthMatches[0]->ordertime_open);

	m_match_count = 0;
	m_profit = 0;
	for (const auto& oneMatch : m_monthMatches)
	{
		m_profit += oneMatch->profit;
		m_match_count += oneMatch->quantity;
	}
	m_profitPerMatch = m_profit / m_match_count;
}

std::string CAnalystResultMonth_Real::PrintResult()
{
	std::string timestr = CGlobal::GetTimeStr(m_monthMsSecond / 1000);
	timestr = timestr.substr(0, 8);
	std::string temstr = fmt::format("\t {} \t {} \t {:.2f} \t {:.2f}", timestr.c_str(), m_match_count, m_profit, m_profitPerMatch);
	return temstr;
}
