#pragma once

#include <QDatabase.h>
#include <base_struc.h>
using namespace IBTrader;
using namespace Ice;

// 更新tick到数据库的工具
class CTickUpdator
{
public:
	CTickUpdator();
	virtual ~CTickUpdator(void) { ; };

	void UpdateTickToDB(const ITick& tick);

protected:

	std::map<IQKey, time_t>	m_lastUpdateTime;			// 最后一次更新指数的时间（毫秒）

	// 更新tick表
	void UpdateTickToTickHis(const ITick& tick);

	// 更新kline表
	void UpdateTickToKLine(const ITick& tick);

	void UpdateTickToKLine(const ITick& tick, ITimeType timeType);

	// 检查是否可以更新指数
	bool CanUpdateIndex(const std::string& codeId, ITimeType timeType);

	// 更新m_lastUpdateTime的时间
	void ChgLastUpdateIndexTime(const std::string& codeId, ITimeType timeType);

	// 计算多长时间计算一次指数
	time_t GetTimeSpec(const std::string& codeId, ITimeType timeType);

	SecurityType GetSecurityType(const std::string& codeId);







};
typedef std::shared_ptr<CTickUpdator> TickUpdatorPtr;
