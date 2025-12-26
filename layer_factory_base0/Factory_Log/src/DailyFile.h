#pragma once

#include "MyLogDefine.h"
#include <memory>
#include <string>
class CDailyFile
{
public:
	CDailyFile(const std::string& logPath);
	virtual ~CDailyFile();

	virtual void		Log_AsyncPrintDailyFile(const std::string& id, const std::string& msg, int modSecond, bool flushNow = false) = 0;

	virtual void		Log_AsyncPrintDailyFile_Flush() = 0;

protected:
	std::string			m_logpath;


};
typedef std::shared_ptr<CDailyFile> DailyFilePtr;


