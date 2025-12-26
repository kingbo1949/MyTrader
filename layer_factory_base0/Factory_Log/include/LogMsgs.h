#pragma once

#include "ToStr.h"

#include <cstring>
enum class LogFileType
{
	DailyFile = 0,				// 每天一份的日志模式
	RotatingFile = 1			// 回卷模式
};

struct OneLogMsg
{
	LogFileType		logFileType = LogFileType::DailyFile;
	char			fileName[200] = {0};
	ToStrPtr		pToStr = nullptr;
	int				modSecond = 1;
	int				flushNow = false;

	OneLogMsg()
	{
		;
	};
	OneLogMsg(LogFileType fileTypeIn, const std::string& fileNameIn, ToStrPtr pToStrIn, int modSecondIn, bool flushNowIn)
	{
		logFileType = fileTypeIn;

#ifdef WIN32
		strcpy_s(fileName, fileNameIn.c_str());
#else
		strcpy(fileName, fileNameIn.c_str());
#endif // WIN32

		pToStr = pToStrIn;
		modSecond = modSecondIn;
		flushNow = flushNowIn;

	};

};

class CLogMsgs
{
public:
	CLogMsgs();
	virtual ~CLogMsgs();

	virtual void			AddMsg(const OneLogMsg& onemsg) = 0;

	virtual void 			HandleAllMsgs() = 0;


};
typedef std::shared_ptr<CLogMsgs> LogMsgsPtr;

