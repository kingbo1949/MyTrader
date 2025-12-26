#pragma once
#include "DailyFile.h"
#include <map>
#include <mutex>
class CDailyFile_Real : public CDailyFile
{
public:
	CDailyFile_Real(const std::string& logPath);
	virtual ~CDailyFile_Real();

	virtual void				Log_AsyncPrintDailyFile(const std::string& id, const std::string& msg, int modSecond, bool flushNow = false) override final;

	virtual void				Log_AsyncPrintDailyFile_Flush() override final;


protected:
	std::mutex							m_mutex;
	std::map<std::string, LoggerPtr>	m_mapLoggers;

	LoggerPtr					MakeAndGet_Logger(const std::string& loggerName, bool& firstUse);

	std::string					GetFirstLine(const std::string& loggerName);
};


