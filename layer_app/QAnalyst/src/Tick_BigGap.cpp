#include "stdafx.h"
#include "Tick_BigGap.h"
#include <Log.h>
#include <Factory_Log.h>
#include <Global.h>
CTick_BigGap::CTick_BigGap(const IBTickPtrs& ticks, int bigGapStep)
	:m_ticks(ticks), m_bigGapStep(bigGapStep)
{
}

void CTick_BigGap::Scan()
{
	for (auto i = 0; i < m_ticks.size(); ++i)
	{
		IBTickPtr tick = m_ticks[i];
		if (tick->bidAsks.size() != 5) continue;

		if (tick->bidAsks[3].bid - tick->bidAsks[4].bid >= m_bigGapStep)
		{
			Gap gap;
			gap.index = i;
			gap.step = tick->bidAsks[3].bid - tick->bidAsks[4].bid;
			m_bidGaps.push_back(gap);
		}

		if (tick->bidAsks[4].ask - tick->bidAsks[3].ask >= m_bigGapStep)
		{
			Gap gap;
			gap.index = i;
			gap.step = tick->bidAsks[4].ask - tick->bidAsks[3].ask;
			m_askGaps.push_back(gap);
		}
	}
}

void CTick_BigGap::PrintToFile(BidOrAsk bidOrAsk)
{
	// 时间 high low close
	std::string fileName = "";
	if (bidOrAsk == BidOrAsk::Bid)
	{
		fileName = "log/bidGaps.csv";
	}
	else
	{
		fileName = "log/askGaps.csv";
	}

	CLog::Instance()->DelLogFile(fileName);
	std::string tital = "time, gap \n";
	CLog::Instance()->PrintStrToFile(fileName, tital);

	std::vector<Gap>& gaps = m_bidGaps;
	if (bidOrAsk == BidOrAsk::Ask)
	{
		gaps = m_askGaps;
	}

	std::string outputStr = "";
	for (auto oneGap : gaps)
	{
		Tick_T time = m_ticks[oneGap.index]->time;
		std::string str = fmt::format("{}, {},  \n",
			CGlobal::GetTickTimeStr(time).c_str(),
			oneGap.step
		);
		outputStr += str;

	}
	CLog::Instance()->PrintStrToFile(fileName, outputStr);

}
