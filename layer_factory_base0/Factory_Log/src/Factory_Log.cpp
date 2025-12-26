#include "pch.h"
#include "Factory_Log.h"
#include "PrintMsgOnce_Real.h"


#include "DailyFile_Real.h"
#include "RotatingFile_Real.h"
#include "LogMsgs_Real.h"

DailyFilePtr	m_pLogDailyFile = nullptr;
RotatingFilePtr m_pLogRotatingFile = nullptr;

FACTORY_LOG_API	void Make_Log(const std::string& logpath /* "./Log/" */)
{
	if (!m_pLogDailyFile)
	{
		m_pLogDailyFile = std::make_shared<CDailyFile_Real>(logpath);
	}
	if (!m_pLogRotatingFile)
	{
		m_pLogRotatingFile = std::make_shared<CRotatingFile_Real>(logpath);
	}
	Log_Print(LogLevel::Info, "--");
	Log_PrintOnce(LogLevel::Info, "**");

	MakeAndGet_LogMsgs();
	return;
}


FACTORY_LOG_API	void Log_Print(LogLevel level, const std::string& msg)
{
	switch (level)
	{
	case LogLevel::Info:
		spdlog::info("{}", msg.c_str());
		break;
	case LogLevel::Warn:
		spdlog::warn("{}", msg.c_str());
		break;
	case LogLevel::Err:
		spdlog::error("{}", msg.c_str());
		break;
	case LogLevel::Critical:
		spdlog::critical("{}", msg.c_str());
		break;
	default:
		break;
	}
	return;
}

FACTORY_LOG_API	void Log_Print2(LogLevel level, const std::string& msg, const std::string& fileName)
{
	Log_Print(level, msg);
	Log_AsyncPrintRotatingFile(fileName, msg, 1, false);
	return;

}

PrintMsgOncePtr m_pPrintMsgOne = nullptr;
FACTORY_LOG_API void Log_PrintOnce(LogLevel level, const std::string& msg)
{
	if (!m_pPrintMsgOne)
	{
		m_pPrintMsgOne = std::make_shared<CPrintMsgOnce_Real>();
	}
	m_pPrintMsgOne->PrintOnce(level, msg);
	return;
}

FACTORY_LOG_API void Log_AsyncPrintDailyFile(const std::string& id, const std::string& msg, int modSecond, bool flushNow)
{
	if (!m_pLogDailyFile)
	{
		Log_Print(LogLevel::Critical, "m_pLogDailyFile is null");
		exit(-1);
	}
	m_pLogDailyFile->Log_AsyncPrintDailyFile(id, msg, modSecond, flushNow);

	return;

}

FACTORY_LOG_API void Log_AsyncPrintRotatingFile(const std::string& fileName, const std::string& msg, int modSecond, bool flushNow)
{
	if (!m_pLogRotatingFile)
	{
		Log_Print(LogLevel::Critical, "m_pLogRotatingFile is null");
		exit(-1);
	}
	m_pLogRotatingFile->Log_AsyncPrintRotatingFile(fileName, msg, modSecond, flushNow);

	return;

}


FACTORY_LOG_API void Log_Close()
{
	spdlog::drop_all();
	spdlog::shutdown();
	return;
}

FACTORY_LOG_API void Log_Flush()
{
	if (m_pLogDailyFile)
	{
		m_pLogDailyFile->Log_AsyncPrintDailyFile_Flush();
	}
	if (m_pLogRotatingFile)
	{
		m_pLogRotatingFile->Log_AsyncPrintRotatingFile_Flush();
	}
	return;
}

LogMsgsPtr g_logMsgs = nullptr;
FACTORY_LOG_API LogMsgsPtr MakeAndGet_LogMsgs()
{
	if (!g_logMsgs)
	{
		g_logMsgs = std::make_shared<CLogMsgs_Real>();
	}
	return g_logMsgs;
}
