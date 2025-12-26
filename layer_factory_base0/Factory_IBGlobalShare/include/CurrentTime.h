#pragma once

#include <memory>
#include <string>
#include <base_struc.h>


class CCurrentTime
{
public:
	CCurrentTime();
	virtual ~CCurrentTime();

	// 设置当前时间（微妙）
	virtual		void			SetCurrentTime(MicroSecond_T microSecond) = 0;

	// 取当前时间（秒）
	virtual		time_t			GetCurrentTime_second() = 0;

	// 取当前时间（毫秒）
	virtual		time_t			GetCurrentTime_millisecond() = 0;

	// 取当前时间（微秒）
	virtual		time_t			GetCurrentTime_microsecond() = 0;

	// 取当前时间（纳秒）
	virtual		time_t			GetCurrentTime_nanoseconds() = 0;

	

	// 取当前时间字符串
	virtual		std::string		GetCurrentTimeStr();

	// 取当前时间时分秒
	virtual		bool			GetCurrentHHMMSS(int& hour, int& min, int& second);
};

typedef std::shared_ptr<CCurrentTime> CurrentTimePtr;

