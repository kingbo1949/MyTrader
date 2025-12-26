#pragma once
#include "DaemonByTick.h"
class CDaemonByTick_Real : public CDaemonByTick
{
public:
	CDaemonByTick_Real():m_lastUpdateSecond_log(0){ ; };
	virtual ~CDaemonByTick_Real() { ; };

protected:
	time_t				m_lastUpdateSecond_log;

	void				Handle_Log();

};

