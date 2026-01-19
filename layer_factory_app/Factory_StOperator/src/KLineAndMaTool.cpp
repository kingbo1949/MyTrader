#include "pch.h"
#include "KLineAndMaTool.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Global.h>
#include <Factory_Log.h>
#include <Factory_IBJSon.h>



IBKLinePtr CKLineAndMaTool::GetLastKline(const std::string& codeId, Time_Type timeType, time_t tickTime)
{
	// 当前时点
	time_t recordNo = CGetRecordNo::GetRecordNo(timeType, tickTime);

	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = recordNo - 1;		// 不包括指定时点
	query.query_number = 1;				// 只取一个数据即可

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	if (klines.empty()) return nullptr;

	return klines[0];

}

IBKLinePtr CKLineAndMaTool::GetOneKLine(const std::string& codeId, Time_Type timeType, time_t ticktime)
{
	time_t kline_time = CGetRecordNo::GetRecordNo(timeType, ticktime);

	IBKLinePtr ret = MakeAndGet_QDatabase()->GetOneKLine(codeId, timeType, kline_time);
	if (!ret)
	{
		//Log_Print(LogLevel::Info, fmt::format("timetype = {}", CTransToStr::Get_TimeType(timeType).c_str()));
		//Log_Print(LogLevel::Err, fmt::format("Kline is null, currentTickTime = {}, recordNo = {}",
		//	CGlobal::GetTickTimeStr(ticktime).c_str(),
		//	CGlobal::GetTickTimeStr(kline_time).c_str()
		//));

	}
	return ret;
}

IBMacdPtr CKLineAndMaTool::GetOneMacd(const std::string &codeId, Time_Type timeType, time_t ticktime)
{
	time_t kline_time = CGetRecordNo::GetRecordNo(timeType, ticktime);

	IBMacdPtr ret = MakeAndGet_QDatabase()->GetOneMacd(codeId, timeType, kline_time);
	return ret;
}

IBDivTypePtr CKLineAndMaTool::GetOneDivType(const std::string &codeId, Time_Type timeType, time_t ticktime)
{
	time_t kline_time = CGetRecordNo::GetRecordNo(timeType, ticktime);

	IBDivTypePtr ret = MakeAndGet_QDatabase()->GetOneDivType(codeId, timeType, kline_time);
	return ret;
}

IBKLinePtr CKLineAndMaTool::GetLastDayCloseKLine(const std::string& codeId, Time_Type timeType, time_t ticktime)
{
	// 取时点所在日的零点
	time_t dayMillisec = CHighFrequencyGlobalFunc::GetDayMillisec(ticktime);

	// 取日零点之前的第一根BAR
	size_t count = 1;
	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = dayMillisec;		// 包括指定时点
	query.query_number = count;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	if (klines.size() < count)
	{
		// 数据不充足，返回空集
		Log_Print(LogLevel::Err, "there is no enough data");
		exit(-1);
	}
	return klines.back();

}

IBKLinePtrs CKLineAndMaTool::GetKLines_OneDay(const std::string& codeId, Time_Type timeType, time_t ticktime, size_t count)
{
	time_t kline_time = CGetRecordNo::GetRecordNo(timeType, ticktime);

	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = kline_time - 1;		// 不包括指定时点
	query.query_number = count;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	if (klines.empty()) return klines;

	if (klines[0]->time > CHighFrequencyGlobalFunc::GetDayMillisec(ticktime) && klines.size() == count)
	{
		return klines;
	}

	// 数据不充足，跨日了, 返回空集
	klines.clear();
	return klines;


}

IBKLinePtrs CKLineAndMaTool::GetKLines_MoreDay(const std::string& codeId, Time_Type timeType, time_t ticktime, size_t count)
{
	time_t kline_time = CGetRecordNo::GetRecordNo(timeType, ticktime);

	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = kline_time - 1;		// 不包括指定时点
	query.query_number = count;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	if (klines.size() < count)
	{
		// 数据不充足，返回空集
		klines.clear();
		return klines;

	}
	else
	{
		return klines;
	}


}



IBKLinePtrs CKLineAndMaTool::GetKLines_MoreDay(const std::string& codeId, Time_Type timeType, const TimePair& timePair)
{
	IBKLinePtrs klines;

	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = CGetRecordNo::GetRecordNo(timeType, timePair.beginPos);
	query.time_pair.endPos = CGetRecordNo::GetRecordNo(timeType, timePair.endPos) - 1; // 不包括endPos所属于的bar

	if (query.time_pair.endPos < query.time_pair.beginPos) return klines;

	klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	return klines;

}

HighAndLow CKLineAndMaTool::GetExtremePrice(const IBKLinePtrs klines)
{
	HighAndLow back;
	for (auto onekline : klines)
	{
		if (onekline->high > back.high)
		{
			back.high = onekline->high;
			back.highPos = onekline->time;
		}
		if (onekline->low < back.low)
		{
			back.low = onekline->low;
			back.lowPos = onekline->time;
		}
	}
	return back;
}

std::string CKLineAndMaTool::GetKLineStr(IBKLinePtr kline)
{
	std::string str = fmt::format("{}, open = {}, close = {}, high = {}, low = {}",
		CGlobal::GetTickTimeStr(kline->time).c_str(),
		kline->open,
		kline->close,
		kline->high,
		kline->low

	);
	return str;

}



bool CKLineAndMaTool::IsFirstKline(const CodeStr& codeId, Time_Type timeType, time_t tickTime)
{
	IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timeType, tickTime);
	if (!thisKline) return false;

	IBKLinePtr lastKline = CKLineAndMaTool::GetLastKline(codeId, timeType, tickTime);
	if (!lastKline)
	{
		Log_Print(LogLevel::Err, "in IsFirstKline, lastKline is null");
		return false;
	}

	time_t diffTime = thisKline->time - lastKline->time;
	if (diffTime < 5 * 3600 * 1000)
	{
		// 当前k线与上一k线时间间隔超过5小时, 否则不是第一根线
		return false;
	}
	return true;


}


time_t CKLineAndMaTool::GetFirstKline(const CodeStr& codeId, Time_Type timeType, time_t tickTime)
{
	// 取时点所在日的零点
	time_t dayMillisec = CHighFrequencyGlobalFunc::GetDayMillisec(tickTime);

	// 取日零点之后的第一根BAR
	size_t count = 1;
	QQuery query;
	query.query_type = QQueryType::AFTER_TIME;
	query.time_ms = dayMillisec;		// 包括指定时点
	query.query_number = count;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	if (klines.size() < count)
	{
		// 数据不充足，返回空集
		Log_Print(LogLevel::Err, "there is no enough data");
		exit(-1);
	}
	return klines.back()->time;
}

time_t CKLineAndMaTool::GetLastDayFirstKline(const CodeStr& codeId, Time_Type timeType, time_t tickTime)
{
	// 取上一日收盘k线
	IBKLinePtr lastDayCloseKline = GetLastDayCloseKLine(codeId, timeType, tickTime);

	return GetFirstKline(codeId, timeType, lastDayCloseKline->time);
}





bool CKLineAndMaTool::GetTodayExtremePrice(const std::string& codeId, Time_Type timeType, time_t ticktime, HighAndLow& highlow)
{
	// 得到当日开盘时间
	time_t dayBeginTime = GetFirstKline(codeId, timeType, ticktime);
	time_t currentBarTime = CGetRecordNo::GetRecordNo(timeType, ticktime);
	if (dayBeginTime >= currentBarTime)
	{
		// 处于开盘bar上
		return false;
	}

	TimePair timePair;
	timePair.beginPos = dayBeginTime;
	timePair.endPos = currentBarTime;
	IBKLinePtrs klines = GetKLines_MoreDay(codeId, timeType, timePair);

	highlow = GetExtremePrice(klines);
	return true;

}

bool CKLineAndMaTool::IsSameDay(const std::string& codeId, Time_Type timeType, time_t ticktime1, time_t ticktime2)
{
	time_t fistKlineTime1 = GetFirstKline(codeId, timeType, ticktime1);
	time_t fistKlineTime2 = GetFirstKline(codeId, timeType, ticktime2);

	if (fistKlineTime1 == fistKlineTime2) return true;
	return false;
}

