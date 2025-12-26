#include "pch.h"
#include "CurrentTime_Simulation.h"

CCurrentTime_Simulation::CCurrentTime_Simulation(MicroSecond_T microSecond)
:m_currentMicroSecond(microSecond)
{
}


CCurrentTime_Simulation::~CCurrentTime_Simulation()
{
}

void CCurrentTime_Simulation::SetCurrentTime(MicroSecond_T microSecond)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_currentMicroSecond = microSecond;
}

time_t CCurrentTime_Simulation::GetCurrentTime_second()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_currentMicroSecond / 1000000;

}

time_t CCurrentTime_Simulation::GetCurrentTime_millisecond()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_currentMicroSecond / 1000;
}

time_t CCurrentTime_Simulation::GetCurrentTime_microsecond()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_currentMicroSecond;
}

time_t CCurrentTime_Simulation::GetCurrentTime_nanoseconds()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_currentMicroSecond * 1000;

}

