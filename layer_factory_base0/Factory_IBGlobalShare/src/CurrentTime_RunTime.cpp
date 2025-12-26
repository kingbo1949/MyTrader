#include "pch.h"
#include "CurrentTime_RunTime.h"
#include <Global.h>

CCurrentTime_RunTime::CCurrentTime_RunTime(MicroSecond_T microSecond)
{
}


CCurrentTime_RunTime::~CCurrentTime_RunTime()
{
}

void CCurrentTime_RunTime::SetCurrentTime(MicroSecond_T microSecond)
{
	// 实盘非仿真，不需要设置当前时间，用机器时间即可
	return;
}

time_t CCurrentTime_RunTime::GetCurrentTime_second()
{
	return benchmark_second();
}

time_t CCurrentTime_RunTime::GetCurrentTime_millisecond()
{
	return benchmark_milliseconds();
}

time_t CCurrentTime_RunTime::GetCurrentTime_microsecond()
{
	return benchmark_microsecond();
}

time_t CCurrentTime_RunTime::GetCurrentTime_nanoseconds()
{
	return benchmark_nanoseconds();

}
