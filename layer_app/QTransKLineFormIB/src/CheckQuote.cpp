#include "CheckQuote.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Factory_Log.h>
#include <Global.h>
CCheckQuote::CCheckQuote(IbContractPtr contract, const TimePair& timePair)
	:m_contract(contract), m_timePair(timePair)
{
	if (m_contract->securityType != SecurityType::FUT)
	{
		// 因为股票盘后数据不计入日K线（TSLA 20240429）,不能采用当前方法检查K线
		Log_Print(LogLevel::Err, "only fut can checkquote");
		exit(-1);
	}

	InitSpecailDay();


}

Errklines CCheckQuote::CheckKlines()
{
	Errklines ret;
	Errklines tem;

	tem = CheckKlines(Time_Type::M1);
	copy(tem.begin(), tem.end(), back_inserter(ret));
	tem.clear();

	tem = CheckKlines(Time_Type::M5);
	copy(tem.begin(), tem.end(), back_inserter(ret));
	tem.clear();

	tem = CheckKlines(Time_Type::M15);
	copy(tem.begin(), tem.end(), back_inserter(ret));
	tem.clear();

	tem = CheckKlines(Time_Type::M30);
	copy(tem.begin(), tem.end(), back_inserter(ret));
	tem.clear();

	tem = CheckKlines(Time_Type::H1);
	copy(tem.begin(), tem.end(), back_inserter(ret));
	tem.clear();

	tem = CheckKlines(Time_Type::D1);
	copy(tem.begin(), tem.end(), back_inserter(ret));
	tem.clear();

	for (const auto& err : ret)
	{
		std::string klinestr = CTransDataToStr::GetKlineStr_Csv(err.kline);
		Log_Print(LogLevel::Warn, fmt::format("klineErr {} {}", CTransToStr::Get_TimeType(err.timeType).c_str(), klinestr.c_str() ));
	}

	return ret;
}

ErrQuotes CCheckQuote::Scan()
{

	time_t beginDayTime = CGetRecordNo::GetRecordNo(Time_Type::D1, m_timePair.beginPos);
	time_t endDayTime = CGetRecordNo::GetRecordNoEnd(Time_Type::D1, m_timePair.endPos);

	ErrQuotes ret;
	for (time_t i=beginDayTime; i<endDayTime; i+= 24 * 60 * 60 * 1000)
	{
		// 因为beginDayTime与endDayTime有可能是不同的夏时制，需要转换统一起来
		i = CGetRecordNo::ModifyDayTimeMs(i);

		std::string timestr = CGlobal::GetTickTimeStr(i);

		ErrQuotes errs = Scan(i);
		if (errs.empty())
		{
			Log_Print(LogLevel::Info, fmt::format("{} has no err", timestr.c_str()));
		}
		else
		{
			copy(errs.begin(), errs.end(), back_inserter(ret));
		}
		
	}
	return ret;

}

ErrQuotes CCheckQuote::Scan(time_t dayMs)
{
	ErrQuotes ret;
	IBKLinePtr daykline = MakeAndGet_QDatabase()->GetOneKLine(m_contract->codeId, Time_Type::D1, dayMs);
	if (!daykline) return ret;

	TimePair timePair = GetDayTimePair(daykline->time);
	if (!CheckDayTimePair(daykline->time, timePair))
	{
		exit(-1);
	}
	std::string beginstr = CGlobal::GetTickTimeStr(timePair.beginPos);
	std::string endstr = CGlobal::GetTickTimeStr(timePair.endPos);

	HighAndLow highLow;
	if (!Scan(daykline, Time_Type::M1, timePair, highLow))
	{
		ret.push_back(MakeErrQuote(daykline, Time_Type::M1, highLow) );
	}
	if (!Scan(daykline, Time_Type::M5, timePair, highLow))
	{
		ret.push_back(MakeErrQuote(daykline, Time_Type::M5, highLow));
	}
	if (!Scan(daykline, Time_Type::M15, timePair, highLow))
	{
		ret.push_back(MakeErrQuote(daykline, Time_Type::M15, highLow));
	}
	if (!Scan(daykline, Time_Type::M30, timePair, highLow))
	{
		ret.push_back(MakeErrQuote(daykline, Time_Type::M30, highLow));
	}
	if (!Scan(daykline, Time_Type::H1, timePair, highLow))
	{
		ret.push_back(MakeErrQuote(daykline, Time_Type::H1, highLow));
	}
	return ret;

}

bool CCheckQuote::Scan(IBKLinePtr dayKLine, Time_Type time_type, TimePair timePair, HighAndLow& highlow)
{
	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = timePair.beginPos;
	query.time_pair.endPos = timePair.endPos + 1;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(m_contract->codeId, time_type, query);
	if (klines.empty()) return false;

	highlow = CHighFrequencyGlobalFunc::MakeHighAndLow(klines, false);
	if (CGlobal::DToI(highlow.high / m_contract->minMove) != CGlobal::DToI(dayKLine->high / m_contract->minMove)) return false;
	if (CGlobal::DToI(highlow.low / m_contract->minMove) != CGlobal::DToI(dayKLine->low / m_contract->minMove)) return false;


	return true;
}

ErrQuote CCheckQuote::MakeErrQuote(IBKLinePtr dayKLine, Time_Type time_type, const HighAndLow& highLow)
{
	ErrQuote ret;
	ret.daykline = dayKLine;
	ret.timeType = time_type;
	ret.highLow = highLow;
	return ret;
}

TimePair CCheckQuote::GetDayTimePair(time_t dayTimeMs)
{
	TimePair timePair;
	// 期货的日K线开盘时间是上一日的18:00:00 - 当日16:59:59
	std::string dayTimeStr = CGlobal::GetTickTimeStr(dayTimeMs);
	dayTimeMs = CHighFrequencyGlobalFunc::GetDayMillisec(dayTimeMs);

	std::map<time_t, TimePair>::const_iterator pos = m_specialDay.find(dayTimeMs);
	if (pos != m_specialDay.end())
	{
		timePair = pos->second;
	}
	else
	{
		timePair.beginPos = (dayTimeMs - 24 * 60 * 60 * 1000) + CHighFrequencyGlobalFunc::MakeMilliSecondPart("18:00:00", 0);
		timePair.endPos = dayTimeMs + CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:59:00", 0);

	}
	return timePair;
}

bool CCheckQuote::CheckDayTimePair(time_t dayTimeMs, const TimePair& timePair)
{
	std::string dayTimeStr = CGlobal::GetTickTimeStr(dayTimeMs);
	IBKLinePtr beginkline = MakeAndGet_QDatabase()->GetOneKLine(m_contract->codeId, Time_Type::M1, timePair.beginPos);
	if (!beginkline)
	{
		std::string beginPosStr = CGlobal::GetTickTimeStr(timePair.beginPos);
		std::string reason = fmt::format("GetDayTimePair {}, can not get beginPos kline {} ", dayTimeStr.c_str(), beginPosStr.c_str());
		Log_Print(LogLevel::Err, reason);
		return false;
	}

	IBKLinePtr endkline = MakeAndGet_QDatabase()->GetOneKLine(m_contract->codeId, Time_Type::M1, timePair.endPos);
	if (!endkline)
	{
		std::string endPosStr = CGlobal::GetTickTimeStr(timePair.endPos);
		std::string reason = fmt::format("GetDayTimePair {}, can not get endPos kline {} ", dayTimeStr.c_str(), endPosStr.c_str());
		Log_Print(LogLevel::Err, reason);
		return false;
	}

	return true;
}

Errklines CCheckQuote::CheckKlines(Time_Type time_type)
{
	Errklines ret;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLineByLoop(m_contract->codeId, time_type, m_timePair);
	if (klines.empty()) return ret;

	for (const auto& kline : klines)
	{
		if (CGlobal::DToI(kline->high / m_contract->minMove) == CGlobal::DToI(kline->low / m_contract->minMove))
		{
			Errkline err;
			err.kline = kline;
			err.timeType = time_type;
			ret.push_back(err);
		}
	}
	return ret;
}

void CCheckQuote::InitSpecailDay()
{
	// 20230529 
	time_t dayTime = CGlobal::GetTimeByStr("20230529 00:00:00") * 1000;
	TimePair timePair;
	timePair.beginPos = CGlobal::GetTimeByStr("20230528 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230529 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20230116 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20230115 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230116 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20230220 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20230219 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230220 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20230407 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20230406 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230407 09:14:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20230619 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20230618 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230619 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20230703 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20230702 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230703 13:14:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20230704 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20230703 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230704 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20230904 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20230903 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20230904 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20231123 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20231122 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20231123 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20231124 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20231123 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20231124 13:14:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20240115 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20240114 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20240115 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;

	dayTime = CGlobal::GetTimeByStr("20240219 00:00:00") * 1000;
	timePair.beginPos = CGlobal::GetTimeByStr("20240218 18:00:00") * 1000;
	timePair.endPos = CGlobal::GetTimeByStr("20240219 12:59:00") * 1000;
	m_specialDay[dayTime] = timePair;
}
