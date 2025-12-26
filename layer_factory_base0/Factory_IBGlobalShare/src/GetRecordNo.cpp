#include "pch.h"
#include "GetRecordNo.h"
#include <Global.h>
#include <Factory_Log.h>
#include "HighFrequencyGlobalFunc.h"
Tick_T CGetRecordNo::GetRecordNo(Time_Type timeType, Tick_T tickTime)
{
	switch (timeType)
	{
	case Time_Type::S15:
		return GetRecordNo_S15(tickTime);
	case Time_Type::M1:
		return GetRecordNo_M1(tickTime);
	case Time_Type::M5:
		return GetRecordNo_M5(tickTime);
	case Time_Type::M15:
		return GetRecordNo_M15(tickTime);
	case Time_Type::M30:
		return GetRecordNo_M30(tickTime);
	case Time_Type::H1:
		return GetRecordNo_H1(tickTime);
	case Time_Type::D1:
		return GetRecordNo_D1(tickTime);
	default:
		break;
	}

	printf("unkown timetype %d \n", int(timeType));
	exit(-1);
	return 0;
}

Tick_T CGetRecordNo::GetRecordNoEnd(Time_Type timeType, Tick_T tickTime)
{
	return GetRecordNo(timeType, tickTime) + GetTimeSpan(timeType);
}
bool CGetRecordNo::TimeInKLine(Time_Type timeType, IBKLinePtr kline, Tick_T time)
{
	Tick_T beginTime = kline->time;
	Tick_T endTime = GetRecordNoEnd(timeType, kline->time);

	if (beginTime <= time && time < endTime) return true;

	return false;

}

bool CGetRecordNo::TimesInSameBar(Time_Type timeType, Tick_T time1, Tick_T time2)
{
	if (GetRecordNo(timeType, time1) == GetRecordNo(timeType, time2)) return true;
	return false;
}

Tick_T CGetRecordNo::GetTimeSpan(Time_Type timeType)
{
	switch (timeType)
	{
	case Time_Type::S15:
		return 15 * 1000;
	case Time_Type::M1:
		return 60 * 1000;
	case Time_Type::M5:
		return 5 * 60 * 1000;
	case Time_Type::M15:
		return 15 * 60 * 1000;
	case Time_Type::M30:
		return 30 * 60 * 1000;
	case Time_Type::H1:
		return 60 * 60 * 1000;
	case Time_Type::D1:
		return 24 * 60 * 60 * 1000;
	default:
		break;
	}

	printf("unkown timetype %d \n", int(timeType));
	exit(-1);
	return 0;
}

Tick_T CGetRecordNo::GetRecordNo_Month(Tick_T tickTime)
{
	// 因为tm_wday和tm_yday的存在所有不能用结构tm赋值的方式来设置为当月1号
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	std::string temstr = fmt::format("{:04d}{:02d}01 00:00:00", 1900 + tt.tm_year, tt.tm_mon + 1);
	return CGlobal::GetTimeByStr(temstr) * 1000;
}

Tick_T CGetRecordNo::GetRecordNoEnd_Month(Tick_T tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	if (tt.tm_mon != 11)
	{
		// 当前不是12月
		std::string temstr = fmt::format("{:04d}{:02d}01 00:00:00", 1900 + tt.tm_year, tt.tm_mon + 1 + 1);
		return CGlobal::GetTimeByStr(temstr) * 1000;
	}
	else
	{
		// 当前是12月
		std::string temstr = fmt::format("{:04d}0101 00:00:00", 1900 + tt.tm_year + 1);
		return CGlobal::GetTimeByStr(temstr) * 1000;
	}
}

Tick_T CGetRecordNo::ModifyDayTimeMs(Tick_T dayTimeMs)
{
	Tick_T ret = dayTimeMs;
	tm tt;
	if (!Trans(dayTimeMs, tt)) return ret;


	if (tt.tm_hour == 1)
	{
		ret = ret - 1 * 60 * 60 * 1000;
	}
	if (tt.tm_hour == 23)
	{
		ret = ret + 1 * 60 * 60 * 1000;
	}
	return ret;

}

Tick_T CGetRecordNo::GetRecordNo_S15(Tick_T tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = tt.tm_sec / 15 * 15;
	return 1000 * (mktime(&tt));

}


Tick_T CGetRecordNo::GetRecordNo_M1(Tick_T tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	return 1000 * (mktime(&tt));
}

Tick_T CGetRecordNo::GetRecordNo_M5(Tick_T tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = tt.tm_min / 5 * 5;
	return 1000 * (mktime(&tt));
}

Tick_T CGetRecordNo::GetRecordNo_M15(Tick_T tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = tt.tm_min / 15 * 15;
	return 1000 * (mktime(&tt));
}

Tick_T CGetRecordNo::GetRecordNo_M30(Tick_T tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = tt.tm_min / 30 * 30;
	return 1000 * (mktime(&tt));
}

Tick_T CGetRecordNo::GetRecordNo_H1(Tick_T tickTime)
{
	tm tt;
	if (!Trans(tickTime, tt)) return 0;

	tt.tm_sec = 0;
	tt.tm_min = 0;
	return 1000 * (mktime(&tt));
}

Tick_T CGetRecordNo::GetRecordNo_D1(Tick_T tickTime)
{
	return CHighFrequencyGlobalFunc::GetDayMillisec(tickTime);

}

bool CGetRecordNo::Trans(Tick_T tickTime, tm& tt)
{
	Second_T secondTime = tickTime / 1000;
	if (!CGlobal::MyLocalTime(&secondTime, &tt))
	{
		Log_Print(LogLevel::Err, "CGetRecordNo::Trans error !");
		exit(-1);
		return false;
	}

	return true;

}
