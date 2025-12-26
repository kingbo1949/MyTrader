#include "pch.h"
#include "LogMsgs_Real.h"
#include "Factory_Log.h"
CLogMsgs_Real::CLogMsgs_Real()
{

}

CLogMsgs_Real::~CLogMsgs_Real()
{

}

void CLogMsgs_Real::AddMsg(const OneLogMsg& onemsg)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	//printf("AddMsg, %s %s\n", onemsg.fileName, onemsg.pToStr->to_str().c_str());
	m_msgs.push_back(onemsg);
	return;
}


void CLogMsgs_Real::HandleAllMsgs()
{
	std::vector<OneLogMsg>	msgs;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_msgs.swap(msgs);
	}
	for (auto onemsg : msgs)
	{
		if (!onemsg.pToStr) continue;;

		//if (onemsg.fileName != "ticks_strategy.log")
		//{
		//	printf("%s %s \n", onemsg.fileName.c_str(), onemsg.pToStr->to_str().c_str());
		//}
		

		if (onemsg.logFileType == LogFileType::DailyFile)
		{
			Log_AsyncPrintDailyFile(onemsg.fileName, onemsg.pToStr->to_str(), onemsg.modSecond, onemsg.flushNow);
		}
		if (onemsg.logFileType == LogFileType::RotatingFile)
		{
			Log_AsyncPrintRotatingFile(onemsg.fileName, onemsg.pToStr->to_str(), onemsg.modSecond, onemsg.flushNow);
		}

	}

	return;
}

