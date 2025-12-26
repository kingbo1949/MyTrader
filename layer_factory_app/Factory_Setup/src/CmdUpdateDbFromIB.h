#pragma once
#include <base_struc.h>
// 在盘中更新k线的线程，不删除数据，也不更新S15周期K线
// 从IB查询K线，更新数据库
class CCmdUpdateDbFromIB
{
public:
	CCmdUpdateDbFromIB(UseType useType): m_useType(useType) { ; };
	virtual ~CCmdUpdateDbFromIB() { ; };

	void				operator()();

protected:
	UseType				m_useType;

	// 从IB查询更新本地数据库
	void				UpdateDbKLineFromIB();

	// 从IB接口取K线，不包括当前K线
	void				UpdateDbKLineFromIB(const std::string& codeId, Time_Type  timeType);

};

