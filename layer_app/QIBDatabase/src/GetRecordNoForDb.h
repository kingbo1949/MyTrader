#pragma once

#include <QDatabase.h>
using namespace IBTrader;
using namespace Ice;

class CGetRecordNoForDb
{
public:
	CGetRecordNoForDb() { ; };
	virtual ~CGetRecordNoForDb() { ; };


	// 计算tick所在的时间标，返回值单位毫秒
	static time_t	GetRecordNo(ITimeType timeType, time_t tickTime);

protected:
	static time_t	GetRecordNo_S15(time_t tickTime);

	static time_t	GetRecordNo_M1(time_t tickTime);

	static time_t	GetRecordNo_M5(time_t tickTime);

	static time_t	GetRecordNo_M15(time_t tickTime);

	static time_t	GetRecordNo_M30(time_t tickTime);

	static time_t	GetRecordNo_H1(time_t tickTime);

	static time_t	GetRecordNo_D1(time_t tickTime);

	static bool		Trans(time_t tickTime, tm& tt);

};

