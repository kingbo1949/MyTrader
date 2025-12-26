#pragma once
#include <base_struc.h>
#include "CurrentTime.h"
#include <mutex>
class CCurrentTime_Simulation : public CCurrentTime
{
public:
	CCurrentTime_Simulation(MicroSecond_T microSecond);
	virtual ~CCurrentTime_Simulation();

	// 设置当前时间（微秒）
	virtual		void			SetCurrentTime(MicroSecond_T microSecond) override final;

	// 取当前时间（秒）
	virtual		time_t			GetCurrentTime_second() override final;


	// 取当前时间（毫秒）
	virtual		time_t			GetCurrentTime_millisecond() override final;

	// 取当前时间（微秒）
	virtual		time_t			GetCurrentTime_microsecond() override final;

	// 取当前时间（纳秒）
	virtual		time_t			GetCurrentTime_nanoseconds() override final;


protected:
	std::mutex					m_mutex;
	MicroSecond_T				m_currentMicroSecond;		// 微妙


};

