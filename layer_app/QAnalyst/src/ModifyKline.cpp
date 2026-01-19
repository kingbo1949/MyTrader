#include "stdafx.h"
#include "ModifyKline.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Global.h>
#include <Log.h>
CModifyKline::CModifyKline(const CodeStr& codeId, Time_Type timeType, IBKLinePtrs klines)
	:m_codeId(codeId), m_timeType(timeType), m_klines(klines)
{
	m_pTradeDay = Make_TradeDayObj(klines);
}

void CModifyKline::CheckHoliday()
{
	std::vector<Tick_T> holidays = NeedEnd();
	for (auto oneday : holidays)
	{
		PadingDay(oneday);
	}


}

void CModifyKline::CheckDayKLineCount()
{
	std::string fileName = "holidays_kline_count.csv";
	std::vector<Tick_T> holidays;
	size_t klinecount = 46;
	for (auto daytime : m_pTradeDay->GetAllTradeDay())
	{
		Tick_T endTime = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("15:30:00", 0);

		QQuery query;
		query.query_type = QQueryType::BEFORE_TIME;
		query.query_number = klinecount;
		query.time_ms = endTime;

		IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(m_codeId, Time_Type::M30, query);
		std::string beginStr = CGlobal::GetTickTimeStr(klines[0]->time);
		std::string endStr = CGlobal::GetTickTimeStr(klines.back()->time);
		if (klines.size() != klinecount)
		{
			holidays.push_back(daytime);
			std::string str = fmt::format("{} != 46", CGlobal::GetTickTimeStr(daytime).c_str());;
			CLog::Instance()->PrintStrToFile(fileName, str + "\n");

			continue;
		}
		if (klines.back()->time != endTime)
		{
			// 终止时间必须是15:30:00
			holidays.push_back(daytime);

			std::string str = fmt::format("{} endtime wrong: {}", CGlobal::GetTickTimeStr(daytime).c_str(), CGlobal::GetTickTimeStr(klines.back()->time).c_str());;
			CLog::Instance()->PrintStrToFile(fileName, str + "\n");
			continue;
		}
		Tick_T beginDayTime = CHighFrequencyGlobalFunc::GetDayMillisec(klines[0]->time);
		if (klines[0]->time - beginDayTime != CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:00:00", 0))
		{
			// 起始时间必须是16:00:00
			holidays.push_back(daytime);

			std::string str = fmt::format("{} begintime wrong: {}", CGlobal::GetTickTimeStr(daytime).c_str(), CGlobal::GetTickTimeStr(klines[0]->time).c_str());
			CLog::Instance()->PrintStrToFile(fileName, str + "\n");

			continue;
		}
	}

	//std::string fileName = "holidays_kline_count.csv";
	//CLog::Instance()->DelLogFile(fileName);
	//for (auto daytime : holidays)
	//{
	//	std::string holidaystr = CGlobal::GetTickTimeStr(daytime);
	//	CLog::Instance()->PrintStrToFile(fileName, holidaystr + "\n");

	//}
}

std::vector<Tick_T> CModifyKline::NeedEnd()
{
	std::vector<Tick_T> holidays;
	std::string fileName = "holidays.csv";
	CLog::Instance()->DelLogFile(fileName);
	for (auto daytime : m_pTradeDay->GetAllTradeDay())
	{
		Tick_T endTime = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("15:30:00", 0);

		size_t klinecount = 1;
		QQuery query;
		query.query_type = QQueryType::BEFORE_TIME;
		query.query_number = klinecount;
		query.time_ms = endTime;

		IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(m_codeId, Time_Type::M30, query);
		if (klines.size() == 0 || klines[0]->time != endTime)
		{
			std::string holidaystr = CGlobal::GetTickTimeStr(daytime);
			holidaystr = holidaystr.substr(0, 8);
			holidays.push_back(daytime);
			CLog::Instance()->PrintStrToFile(fileName, holidaystr + "\n");

		}
	}




	return holidays;
}

void CModifyKline::PadingDay(Tick_T daytime)
{
	Tick_T endTime = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:30:00", 0);

	size_t klinecount = 1;
	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.query_number = klinecount;
	query.time_ms = endTime;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(m_codeId, Time_Type::M30, query);
	Tick_T step = 30 * 60 * 1000;
	Tick_T beginTime = klines[0]->time + step;
	for (Tick_T i = beginTime; i <= endTime; i += step)
	{
		IBKLinePtr kline = std::make_shared<CIBKLine>();
		kline->time = i;
		kline->open = klines[0]->close;
		kline->high = klines[0]->close;
		kline->low = klines[0]->close;
		kline->close = klines[0]->close;
		kline->vol = 0;
		std::string klinetime = CGlobal::GetTickTimeStr(i);
		MakeAndGet_QDatabase()->UpdateKLine(m_codeId, Time_Type::M30, kline);
	}
}

void CModifyKline::CheckVol()
{
	for (auto kline : m_klines)
	{
		if (kline->vol == 0)
		{
			MakeAndGet_QDatabase()->RemoveOneKLine(m_codeId, m_timeType, kline->time);
		}
	}
}


void CModifyKline::AddSpecaildata()
{
	// 20240527 14:30:00 30分钟缺失
	// 20240527 16:00:00 30分钟缺失
	for (auto kline : m_klines)
	{
		if (kline->time != CGlobal::GetTickTimeByStr("20240527 14:00:00.0") && kline->time != CGlobal::GetTickTimeByStr("20240527 15:30:00.0")) continue;;

		IBKLinePtr newKline = kline->Clone();
		newKline->time += 30 * 60 * 1000;
		MakeAndGet_QDatabase()->UpdateKLine(m_codeId, Time_Type::M30, newKline);

	}
	return;

}
