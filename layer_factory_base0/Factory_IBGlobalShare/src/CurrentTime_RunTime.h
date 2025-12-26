#pragma once
#include "CurrentTime.h"
class CCurrentTime_RunTime : public CCurrentTime
{
public:
	CCurrentTime_RunTime(MicroSecond_T microSecond);
	virtual ~CCurrentTime_RunTime();

	// 设置当前时间（微妙）
	virtual		void			SetCurrentTime(MicroSecond_T microSecond) override final;

	// 取当前时间（秒）
	virtual		time_t			GetCurrentTime_second() override final;

	// 取当前时间（毫秒）
	virtual		time_t			GetCurrentTime_millisecond() override final;

	// 取当前时间（微秒）
	virtual		time_t			GetCurrentTime_microsecond() override final;

	// 取当前时间（纳秒）
	virtual		time_t			GetCurrentTime_nanoseconds() override final;


};

