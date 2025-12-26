#include "pch.h"
#include "GetRecordNoForDb.h"
#include <Global.h>
time_t CGetRecordNoForDb::GetRecordNo(ITimeType timeType, time_t tickTime)
{
	switch (timeType)
	{
	case ITimeType::S15:
		return GetRecordNo_S15(tickTime);
	case ITimeType::M1:
		return GetRecordNo_M1(tickTime);
	case ITimeType::M5:
		return GetRecordNo_M5(tickTime);
	case ITimeType::M15:
		return GetRecordNo_M15(tickTime);
	case ITimeType::M30:
		return GetRecordNo_M30(tickTime);
	case ITimeType::H1:
		return GetRecordNo_H1(tickTime);
	case ITimeType::D1:
		return GetRecordNo_D1(tickTime);
	default:
		break;
	}

	printf("unkown timetype %d \n", int(timeType));
	exit(-1);
	return 0;
}


time_t CGetRecordNoForDb::GetRecordNo_S15(time_t tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = tt.tm_sec / 15 * 15;
	return 1000 * (mktime(&tt));
}

time_t CGetRecordNoForDb::GetRecordNo_M1(time_t tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	return 1000 * (mktime(&tt));
}

time_t CGetRecordNoForDb::GetRecordNo_M5(time_t tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = tt.tm_min / 5 * 5;
	return 1000 * (mktime(&tt));
}

time_t CGetRecordNoForDb::GetRecordNo_M15(time_t tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = tt.tm_min / 15 * 15;
	return 1000 * (mktime(&tt));
}

time_t CGetRecordNoForDb::GetRecordNo_M30(time_t tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = tt.tm_min / 30 * 30;
	return 1000 * (mktime(&tt));
}

time_t CGetRecordNoForDb::GetRecordNo_H1(time_t tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = 0;
	return 1000 * (mktime(&tt));
}

time_t CGetRecordNoForDb::GetRecordNo_D1(time_t tickTime)
{
	time_t seconds = tickTime / 1000;
	time_t back = CGlobal::QGetDayTime(seconds) * 1000;
	return back;
}

bool CGetRecordNoForDb::Trans(time_t tickTime, tm& tt)
{
	time_t secondTime = tickTime / 1000;
	if (!CGlobal::MyLocalTime(&secondTime, &tt))
	{
		printf("CGetRecordNo::Trans error !");
		exit(-1);
	}

	return true;

}
