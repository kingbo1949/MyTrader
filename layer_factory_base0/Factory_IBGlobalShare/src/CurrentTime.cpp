#include "pch.h"
#include "CurrentTime.h"
#include <Global.h>

CCurrentTime::CCurrentTime()
{
}


CCurrentTime::~CCurrentTime()
{
}

std::string CCurrentTime::GetCurrentTimeStr()
{
	return CGlobal::GetTimeStr(GetCurrentTime_second());
}

bool CCurrentTime::GetCurrentHHMMSS(int& hour, int& min, int& second)
{
	tm tt = { 0 };

	time_t currentSecond = GetCurrentTime_second();
	if (!CGlobal::MyLocalTime(&currentSecond, &tt)) return false;

	hour = tt.tm_hour;
	min = tt.tm_min;
	second = tt.tm_sec;
	return true;

}
