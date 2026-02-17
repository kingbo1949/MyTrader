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
	// 更新tick表
	void UpdateTickToTickHis(const ITick& tick);

	// 更新kline表
	void UpdateTickToKLine(const ITick& tick);

	void UpdateTickToKLine(const ITick& tick, ITimeType timeType);

	SecurityType GetSecurityType(const std::string& codeId);







};
typedef std::shared_ptr<CTickUpdator> TickUpdatorPtr;
