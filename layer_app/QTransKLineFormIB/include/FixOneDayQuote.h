#pragma once

#include <base_struc.h>
class CFixOneDayQuote
{
public:
	CFixOneDayQuote(IbContractPtr contract, time_t dayTimeMs);
	virtual ~CFixOneDayQuote() { ; };

	void		Go();

protected:
	IbContractPtr			m_contract;
	time_t					m_dayTimeMs;
	IBKLinePtr				m_dayKlineIb;					// IB查询得到的日K线
	std::map<std::string, time_t>		m_errDaysInIBQuote;

	IBKLinePtr				QueryIbForDay();				// 查询IB的日K线
	IBKLinePtrs				QueryIb(Time_Type timeType);	// 查询IB的周期K线

	// 有些数据在IB的数据库中就有错误，比如日线的高低价从未在N周期中出现过，把他们标记出来
	void					InitErrDaysInIBQuote();
	bool					HaveErrInIBQuote(const std::string& codeId, time_t dayTimeMs);



};

