#pragma once

#include <base_struc.h>
#include <base_trade.h>
class CCompareQuote
{
public:
	CCompareQuote(UseType useType) : m_useType(useType) { ; };
	virtual ~CCompareQuote() { ; };

	void		Go();
protected:
	UseType			m_useType;

	// 比较最近count根K线，不包括当前K线
	void			CompareOneCode(const std::string& codeId, Time_Type  timeType, size_t count);

	// 从IB接口取K线，不包括当前K线
	IBKLinePtrs		GetKLine_FromIB_OneCode(const std::string& codeId, Time_Type  timeType, size_t count);
};

