#pragma once




#ifdef WIN32


#ifdef FACTORY_LOG_EXPORTS
#define FACTORY_LOG_API __declspec(dllexport)
#else
#define FACTORY_LOG_API __declspec(dllimport)
#endif


#else

#define FACTORY_LOG_API 

#endif // WIN32


#include <string>
#include "MyLogDefine.h"
#include "LogMsgs.h"
#include "ToStr_Str.h"
#include "ToStr_FunctionObj.h"


#ifndef WIN32
extern "C"
{
#endif // __cplusplus

	FACTORY_LOG_API	void				Make_Log(const std::string& logpath = "./log/");

	FACTORY_LOG_API	void				Log_Print(LogLevel level, const std::string& msg);

	// 打印屏幕并打印文件
	FACTORY_LOG_API	void				Log_Print2(LogLevel level, const std::string& msg, const std::string& fileName);

	// 向屏幕打印字符串，但是5秒之内，相同的字符串只打印一次
	FACTORY_LOG_API void				Log_PrintOnce(LogLevel level, const std::string& msg);

	// 异步打印daily文件， id即是文件名(系统会自动加上扩展名.log)同时也是logger的名称， 文件将在每天凌晨3点建立, modSecond=1则每条打印，否则取模打印
	FACTORY_LOG_API void				Log_AsyncPrintDailyFile(const std::string& id, const std::string& msg, int modSecond, bool flushNow = false);

	// 异步打印Rotating文件， fileName即是文件名同是也是logger的名称， 文件最大10M, modSecond=1则每条打印，否则取模打印
	FACTORY_LOG_API void				Log_AsyncPrintRotatingFile(const std::string& fileName, const std::string& msg, int modSecond, bool flushNow = false);

	FACTORY_LOG_API void				Log_Close();

	FACTORY_LOG_API void				Log_Flush();

	FACTORY_LOG_API LogMsgsPtr			MakeAndGet_LogMsgs();

	class FACTORY_LOG_API				CToStr;

	class FACTORY_LOG_API				CToStr_Str;

	class FACTORY_LOG_API				CToStr_FunctionObj;







#ifndef WIN32
}
#endif // __cplusplus


