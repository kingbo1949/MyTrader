#include "pch.h"
#include "SeriesNoMaker_Real.h"
#include "Factory_IBGlobalShare.h"

CSeriesNoMaker_Real::CSeriesNoMaker_Real()
	:m_seriesNo(0)
{
}

CSeriesNoMaker_Real::~CSeriesNoMaker_Real()
{
}

time_t CSeriesNoMaker_Real::GetSeriesNo()
{
	if (m_seriesNo == 0)
	{
		// 用当日的秒数减去昨日收盘秒数，作为起始no
		time_t seconds = Get_CurrentTime()->GetCurrentTime_second();
		time_t lastCloseSeconds = CHighFrequencyGlobalFunc::MakeLastCloseTime() / 1000;
		seconds = seconds - lastCloseSeconds;
		m_seriesNo = seconds;
	}
	else
	{
		m_seriesNo++;

	}

	return m_seriesNo;

}

void CSeriesNoMaker_Real::SetSeriesNo(time_t seriesNo)
{
	m_seriesNo = seriesNo;

	return;

}
