#pragma once

// 检查数据库行情是否正确，有时日线上的最高最低价格在分钟线上完全没有出现过，这就是导入行情时出现了错误

#include <base_struc.h>

struct ErrQuote
{
	IBKLinePtr		daykline;		// 行情错误的日k线
	Time_Type		timeType;
	HighAndLow		highLow;		// 按timeType周期查询当日的高低价
};
typedef std::vector<ErrQuote> ErrQuotes;

struct Errkline
{
	IBKLinePtr		kline;			// 行情错误的k线
	Time_Type		timeType;
};
typedef std::vector<Errkline> Errklines;


class CCheckQuote
{
public:
	CCheckQuote(IbContractPtr contract, const TimePair& timePair) ;
	virtual ~CCheckQuote() { ; };

	// 每个周期的K线不能高低价格相同
	Errklines		CheckKlines();

	// 日线的高低价必须出现在每个子周期当中
	ErrQuotes		Scan();

protected:
	IbContractPtr	m_contract;
	TimePair		m_timePair;

	std::map<time_t, TimePair> m_specialDay;		// 特定的一些日期，其开盘与收盘时间与众不同，一般都是因为假期放假半天导致

	// 扫描某天
	ErrQuotes 		Scan(time_t dayMs);

	bool			Scan(IBKLinePtr dayKLine, Time_Type time_type, TimePair timePair, HighAndLow& highlow);

	ErrQuote		MakeErrQuote(IBKLinePtr dayKLine, Time_Type time_type, const HighAndLow& highLow);

	// 指定日期零点毫秒，得到该日期1分钟k线的起始种植时间
	TimePair		GetDayTimePair(time_t dayTimeMs);
	bool			CheckDayTimePair(time_t dayTimeMs, const TimePair& timePair);

	Errklines		CheckKlines(Time_Type time_type);

	void			InitSpecailDay();



};

