#pragma once

#include <string>
#include <memory>
#include <mutex>

class CLog;
typedef std::shared_ptr<CLog> LogPtr;


class CLog
{
public:
	CLog();
	virtual ~CLog();

	static LogPtr		Instance();

	// 删除log文件, fileName不含路径
	void				DelLogFile(const std::string& fileName);

	// 打印日志 fileName为-1则不打印到文件,为空打印到缺省文件
	void				PrintLog(std::string fileName, const char* format, ...);

	// 取日志文件名：yyyymmdd.log
	std::string			GetNormalLogFileName();

	void				DebugOutput(const char* format, ...);

	// str打印到文件，不会添加时间戳，也不会添加换行符
	void                PrintStrToFile(std::string fileName, const std::string& str);

	// 把文本文件读到string
	std::string			ReadFileToString(std::string fileName);

	static std::string	vformat_string(const char* fmt, va_list args);




protected:

	static LogPtr		m_pLog;

	int					m_maxLogBuffLength;
	char* m_logBuff;

	std::mutex			m_mutex;




};

