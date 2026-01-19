#pragma once
#include <base_trade.h>
// 与k线有关策略的工具类



class CKLineAndMaTool
{
public:
	CKLineAndMaTool() { ; };
	virtual ~CKLineAndMaTool() { ; };

	// 得到指定时点(毫秒)的K线
	static IBKLinePtr			GetOneKLine(const std::string& codeId, Time_Type timeType, time_t ticktime);

	// 得到指定时点(毫秒)的macd线
	static IBMacdPtr			GetOneMacd(const std::string& codeId, Time_Type timeType, time_t ticktime);

	// 得到指定时点(毫秒)的divtype线
	static IBDivTypePtr			GetOneDivType(const std::string& codeId, Time_Type timeType, time_t ticktime);

	// 得到指定时点的上一个BAR
	static IBKLinePtr			GetLastKline(const std::string& codeId, Time_Type timeType, time_t tickTime);


	// 得到指定时点(毫秒)的上一日的收盘K线
	static IBKLinePtr			GetLastDayCloseKLine(const std::string& codeId, Time_Type timeType, time_t ticktime);

	// 得到指定时点(毫秒)之前(不包括指定时点)的count根K线，k线数目不足，则返回空
	// 仅限当日
	static IBKLinePtrs			GetKLines_OneDay(const std::string& codeId, Time_Type timeType, time_t ticktime, size_t count);

	// 得到指定时点(毫秒)之前(不包括指定时点)的count根K线，k线数目不足，则返回空
	// 可多日
	static IBKLinePtrs			GetKLines_MoreDay(const std::string& codeId, Time_Type timeType, time_t ticktime, size_t count);

	// 得到指定时间区间[timeBegin, timeEnd)的k线, 也就是不包含timeEnd所属于的BAR
	// 可多日
	static IBKLinePtrs			GetKLines_MoreDay(const std::string& codeId, Time_Type timeType, const TimePair& timePair);

	// 返回指定k线集合的高低价格
	static HighAndLow			GetExtremePrice(const IBKLinePtrs klines);

	// k线字符串
	static std::string			GetKLineStr(IBKLinePtr kline);

	// 检查指定时点，是否是当日第一个BAR
	static bool					IsFirstKline(const CodeStr& codeId, Time_Type timeType, time_t tickTime);

	// 查找指定时点当日第一根bar的时点(毫秒)
	static time_t				GetFirstKline(const CodeStr& codeId, Time_Type timeType, time_t tickTime);

	// 查找指定时点上一日第一根bar的时点(毫秒)
	static time_t				GetLastDayFirstKline(const CodeStr& codeId, Time_Type timeType, time_t tickTime);

	// 取ticktime所在当日极值，但不包括当前bar
	// 如果当前bar是开盘bar则返回false
	static bool 				GetTodayExtremePrice(const std::string& codeId, Time_Type timeType, time_t ticktime, HighAndLow& highlow);

	// 是否处于相同日期
	static bool 				IsSameDay(const std::string& codeId, Time_Type timeType, time_t ticktime1, time_t ticktime2);


};

