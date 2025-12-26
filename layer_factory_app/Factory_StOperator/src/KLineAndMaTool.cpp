#include "pch.h"
#include "KLineAndMaTool.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Global.h>
#include <Factory_Log.h>
#include <Factory_IBJSon.h>

double CKLineAndMaTool::GetMa(const IBKLinePtrs& klines)
{
	double ret = 0.0;
	if (klines.size() == 0) return ret;

	for (auto kline : klines)
	{
		ret += kline->close;
	}
	ret = ret / klines.size();

	return ret;
}

IBMaPtr CKLineAndMaTool::GetLastMa(const std::string& codeId, Time_Type timeType, time_t currentTickTime)
{
	// 当前时点
	time_t recordNo = CGetRecordNo::GetRecordNo(timeType, currentTickTime);

	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = recordNo - 1;		// 不包括指定时点
	query.query_number = 1;				// 只取一个数据即可

	IBMaPtrs mas = MakeAndGet_QDatabase()->GetMas(codeId, timeType, query);
	if (mas.empty())
	{
		Log_Print(LogLevel::Err, "last MA is null");
		return nullptr;
	}

	return mas[0];
}

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

HighAndLow CKLineAndMaTool::MakeHighAndLow(const std::string& codeId, Time_Type timeType, time_t ticktime, size_t count, bool useOpenAndClose)
{
	HighAndLow highAndLow;
	IBKLinePtrs klines = GetKLines_MoreDay(codeId, timeType, ticktime, count);
	if (klines.size() == 0) return highAndLow;

	highAndLow = CHighFrequencyGlobalFunc::MakeHighAndLow(klines, useOpenAndClose);

	return highAndLow;
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


bool CKLineAndMaTool::CheckMaForOpen(const CodeStr& codeId, Time_Type timeType, time_t tickTime, BuyOrSell open_buyorsell)
{
	IBMaPtr lastMa = GetLastMa(codeId, timeType, tickTime);
	if (!lastMa) return false;

	IBKLinePtr  kline = GetOneKLine(codeId, timeType, tickTime);
	if (!kline) return false;

	double rate = 0.0;
	if (open_buyorsell == BuyOrSell::Buy && kline->open < lastMa->v20)
	{
		rate = (lastMa->v20 - kline->open) / kline->open;
		Log_Print(LogLevel::Critical, fmt::format("{} open buy, open < ma -------- {}",
			CGlobal::GetTickTimeStr(tickTime).c_str(),
			rate

		));
		return true;
	}

	if (open_buyorsell == BuyOrSell::Sell && kline->open > lastMa->v20)
	{
		rate = (kline->open - lastMa->v20) / kline->open;
		Log_Print(LogLevel::Critical, fmt::format("{} open sell, open > ma -------- {}",
			CGlobal::GetTickTimeStr(tickTime).c_str(),
			rate


		));
		return true;
	}

	//if (CGlobal::IsZero(rate)) return false;
	//if (rate < 0.03) return false;

	return false;

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

BoolType CKLineAndMaTool::FirstKlineAndJump(const CodeStr& codeId, Time_Type timeType, time_t tickTime, double& jumpRate)
{
	if (!IsFirstKline(codeId, timeType, tickTime)) return BoolType::False;


	IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timeType, tickTime);
	IBKLinePtr lastKline = CKLineAndMaTool::GetLastKline(codeId, timeType, tickTime);

	jumpRate = (thisKline->open - lastKline->close) / lastKline->close;
	return BoolType::True;
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

BuyOrSell CKLineAndMaTool::GetMaTrend(const std::string& codeId, Time_Type timeType, time_t currentTickTime)
{
	// 当前时点
	time_t recordNo = CGetRecordNo::GetRecordNo(timeType, currentTickTime);

	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = recordNo - 1;		// 不包括指定时点
	query.query_number = 5;				// 取5个数据

	IBMaPtrs mas = MakeAndGet_QDatabase()->GetMas(codeId, timeType, query);
	if (mas.empty())
	{
		Log_Print(LogLevel::Err, "GetMaTrend MA is null");
		exit(-1);
	}

	if (mas[0]->v20 > mas.back()->v20) return BuyOrSell::Sell;
	return BuyOrSell::Buy;

}


bool CKLineAndMaTool::JumpDay_Unfavorable(const std::string& codeId, Time_Type timeType, time_t tickTime, double jumpRate, BuyOrSell wantoToBuyOrSell)
{
	time_t firstbar = CKLineAndMaTool::GetFirstKline(codeId, timeType, tickTime);

	// 检查跳空幅度，需要足够大
	double realJumpRate = 0.0;
	BoolType boolType = CKLineAndMaTool::FirstKlineAndJump(codeId, timeType, firstbar, realJumpRate);
	if (boolType != BoolType::True) return false;

	if (abs(realJumpRate) < jumpRate)
	{
		// 跳空幅度太小
		return false;
	}
	// 检查是否不利
	if (realJumpRate > 0 && wantoToBuyOrSell == BuyOrSell::Sell)
	{
		// 上跳空且准备卖出 这是有利跳空
		return false;
	}
	if (realJumpRate < 0 && wantoToBuyOrSell == BuyOrSell::Buy)
	{
		// 下跳空且准备买入 这是有利跳空
		return false;
	}
	return true;

}

bool CKLineAndMaTool::JumpDay_Favorable(const std::string& codeId, Time_Type timeType, time_t tickTime, double jumpRate, BuyOrSell wantoToBuyOrSell)
{
	time_t firstbar = CKLineAndMaTool::GetFirstKline(codeId, timeType, tickTime);

	// 检查跳空幅度，需要足够大
	double realJumpRate = 0.0;
	BoolType boolType = CKLineAndMaTool::FirstKlineAndJump(codeId, timeType, firstbar, realJumpRate);
	if (boolType != BoolType::True) return false;


	if (abs(realJumpRate) < jumpRate)
	{
		// 跳空幅度太小
		return false;
	}
	// 检查是否有利
	if (realJumpRate > 0 && wantoToBuyOrSell == BuyOrSell::Buy)
	{
		// 上跳空且准备买入 这是不利跳空
		return false;
	}
	if (realJumpRate < 0 && wantoToBuyOrSell == BuyOrSell::Sell)
	{
		// 下跳空且准备卖出 这是不利跳空
		return false;
	}
	return true;

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

