#pragma once
#include "RotatingFile.h"
#include <map>
#include <mutex>


class CRotatingFile_Real : public CRotatingFile
{
public:
	CRotatingFile_Real(const std::string& logPath);
	virtual ~CRotatingFile_Real();

	virtual void				Log_AsyncPrintRotatingFile(const std::string& fileName, const std::string& msg, int modSecond, bool flushNow = false) override final;

	virtual void				Log_AsyncPrintRotatingFile_Flush() override final;

protected:
	std::mutex								m_mutex;
	std::map<std::string, LoggerPtr>		m_mapLoggers;
	

	LoggerPtr					MakeAndGet_Logger(const std::string& loggerName);

};

