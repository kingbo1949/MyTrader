#include "pch.h"
#include "RotatingFile_Real.h"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "Factory_Log.h"
CRotatingFile_Real::CRotatingFile_Real(const std::string& logPath)
	:CRotatingFile(logPath)
{

}

CRotatingFile_Real::~CRotatingFile_Real()
{

}

void CRotatingFile_Real::Log_AsyncPrintRotatingFile(const std::string& fileName, const std::string& msg, int modSecond, bool flushNow /*= false*/)
{
	auto logger = MakeAndGet_Logger(fileName);
	if (!logger)
	{
		exit(-1);
	}

	// 开始输出
	if (modSecond == 1 || time(nullptr) % modSecond == 0)
	{
		logger->info("{}", msg.c_str());
		if (flushNow)
		{
			logger->flush();
		}
	}


}

void CRotatingFile_Real::Log_AsyncPrintRotatingFile_Flush()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto oneItem : m_mapLoggers)
	{
		oneItem.second->flush();
	}
	return;
}

LoggerPtr CRotatingFile_Real::MakeAndGet_Logger(const std::string& loggerName)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	try
	{
		std::map<std::string, LoggerPtr>::const_iterator pos = m_mapLoggers.find(loggerName);
		if (pos != m_mapLoggers.end())
		{
			return pos->second;
		}

		std::string fullPathFileName = fmt::format("{}{}", m_logpath.c_str(), loggerName.c_str());

		// 10M大小，一个文件
		LoggerPtr logger = spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(loggerName, fullPathFileName, 1048576 * 10, 0);
		logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");

		m_mapLoggers[loggerName] = logger;

		return logger;

	}
	catch (const spdlog::spdlog_ex& ex)
	{
		Log_Print(LogLevel::Critical, ex.what());
		return nullptr;
	}

}
