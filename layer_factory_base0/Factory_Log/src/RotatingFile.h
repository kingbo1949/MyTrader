#pragma once

#include "MyLogDefine.h"
#include <memory>
#include <string>

class CRotatingFile
{
public:
	CRotatingFile(const std::string& logPath);
	virtual ~CRotatingFile();

	virtual void		Log_AsyncPrintRotatingFile(const std::string& fileName, const std::string& msg, int modSecond, bool flushNow = false) = 0;

	virtual void		Log_AsyncPrintRotatingFile_Flush() = 0;

protected:
	std::string			m_logpath;


};
typedef std::shared_ptr<CRotatingFile> RotatingFilePtr;

