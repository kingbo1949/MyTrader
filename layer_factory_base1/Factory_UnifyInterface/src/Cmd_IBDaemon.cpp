#include "pch.h"
#include "Cmd_IBDaemon.h"
#include "Factory_UnifyInterface.h"
#include <Factory_Log.h>
CCmd_IBDaemon::CCmd_IBDaemon(EClientSocket* const pApi, EReader* pReader, EReaderOSSignal* pOsSignal)
	:m_pApi(pApi), m_pReader(pReader), m_pOsSignal(pOsSignal)
{

}

void CCmd_IBDaemon::operator()()
{
	while (Get_Sys_Status() == Thread_Status::Running)
	{
		if (!m_pApi->isConnected())
		{
			Log_Print(LogLevel::Critical, "Connection is broken");
			exit(-1);
		}
		ProcessMessages();
	}

	Log_Print(LogLevel::Warn, "Thread CCmdQGernerator_RealTime is over");
	return;


}

void CCmd_IBDaemon::ProcessMessages()
{
	// 即便无行情的非开盘状态，IB也会每2秒发送一个signal来触发本函数而不会被堵塞
	m_pOsSignal->waitForSignal();
	m_pReader->processMsgs();

}
