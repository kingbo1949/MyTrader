#include "TableModel_Ma_Mouse.h"
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
CTableModel_Ma_Mouse::CTableModel_Ma_Mouse()
	:CTableModel_Ma()
{
}

void CTableModel_Ma_Mouse::SetMouseTime(Time_Type timetype, Tick_T mouseTime)
{
	m_mouseTimetype = timetype;
	m_mouseTime = mouseTime;
	return;
}

void CTableModel_Ma_Mouse::QueryClose()
{
	m_close = std::numeric_limits<double>::quiet_NaN();

	m_mouseKline = MakeAndGet_QDatabase()->GetOneKLine(m_codeId, m_mouseTimetype, m_mouseTime);
	if (!m_mouseKline) return;

	m_close = m_mouseKline->close;
	return;

}

void CTableModel_Ma_Mouse::QueryData(Time_Type timetype)
{
	if (static_cast<int>(timetype) < static_cast<int>(m_mouseTimetype))
	{
		return;
	}

	AverageValue mavalue;
	mavalue.v5 = CountAverage(m_codeId, timetype, TransMouseKline(timetype), 5);
	mavalue.v20 = CountAverage(m_codeId, timetype, TransMouseKline(timetype), 20);
	mavalue.v60 = CountAverage(m_codeId, timetype, TransMouseKline(timetype), 60);
	mavalue.v200 = CountAverage(m_codeId, timetype, TransMouseKline(timetype), 200);
	//if (std::isnan(mavalue.v5) || std::isnan(mavalue.v20) || std::isnan(mavalue.v60) || std::isnan(mavalue.v200))
	//{
	//	return;
	//}
	m_mapMa[timetype] = mavalue;
	return;

}

double CTableModel_Ma_Mouse::CountAverage(const std::string& codeId, Time_Type timeType, IBKLinePtr newKline, int circle)
{
	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.query_number = circle - 1;
	query.time_ms = newKline->time - 10;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	klines.push_back(newKline);
	if (klines.size() != circle)
	{
		return std::numeric_limits<double>::quiet_NaN();
	}
	auto sumClose = [](double acc, const IBKLinePtr& kline)
		{
			return acc + kline->close;
		};

	double sum = std::accumulate(klines.begin(), klines.begin() + circle, 0.0, sumClose);
	return sum / circle;
}

IBKLinePtr CTableModel_Ma_Mouse::TransMouseKline(Time_Type timeType)
{
	if (!m_mouseKline) return nullptr;

	IBKLinePtr ret = m_mouseKline->Clone();
	ret->time = CGetRecordNo::GetRecordNo(timeType, m_mouseKline->time);
	return ret;


	
}
