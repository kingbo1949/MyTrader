#include "pch.h"
#include "DaemonByTick_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>

void CDaemonByTick_Real::Handle_Log()
{
	if (Get_CurrentTime()->GetCurrentTime_second() - m_lastUpdateSecond_log > 5)
	{
		// 5秒执行一次
		MakeAndGet_LogMsgs()->HandleAllMsgs();
		Log_Flush();

		m_lastUpdateSecond_log = Get_CurrentTime()->GetCurrentTime_second();
	}
	return;

}
