// #include "pch.h"
#include "Log.h"
#include <stdarg.h>

#include <boost/filesystem.hpp>
using namespace boost::filesystem;


#include "Global.h"




#ifdef _WIN32

#include <windows.h>
// windows编译需要
#ifdef _DEBUG
#define CON_PRINTF DebugOutput
#else
#define CON_PRINTF printf
#endif
#define MY_VSNPRINTF(buff,len,format,argptr)    _vsnprintf_s((buff),(len),(len-1),(format),(argptr))
#define MY_VPRINTF       _vscprintf
#else
// linux需要
#define CON_PRINTF printf
#define MY_VSNPRINTF    vsnprintf
#define MY_VPRINTF      vprintf
#endif

#ifndef _WIN32
// linux
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef char _TCHAR;
#define ZeroMemory(destination,length) memset((destination),0,(length))
#define _I64_MAX 9223372036854775807
#define _atoi64 atoll
#endif


#define LOG_DIR "log/"

CLog::CLog()
	:m_maxLogBuffLength(0), m_logBuff(nullptr), m_maxDbgLen(0), m_dbgOutputBuff(nullptr)
{

}

LogPtr CLog::m_pLog = NULL;

void CLog::DebugOutput(const char* format, ...)
{
	va_list argList;

	va_start(argList, format);
#ifdef _WIN32
	int  str_count = MY_VPRINTF(format, argList);

	//打印数据超过缓冲区长度，则增加缓冲区的长度
	if (m_maxDbgLen <= str_count)
	{
		delete m_dbgOutputBuff;
		m_maxDbgLen = str_count * 2;
		m_dbgOutputBuff = new char[m_maxDbgLen];
	}

	ZeroMemory(m_dbgOutputBuff, m_maxDbgLen);
	//_vsnprintf_s 长度超过的时候会弄出个断言。
	MY_VSNPRINTF(m_dbgOutputBuff, m_maxDbgLen, format, argList);
	m_dbgOutputBuff[m_maxDbgLen - 1] = '\0';
#else
	printf(format, argList);
#endif
	va_end(argList);

#ifdef _DEBUG
	OutputDebugStringA(m_dbgOutputBuff);
#else
	printf("%s", m_dbgOutputBuff);
#endif

}

void CLog::PrintStrToFile(std::string fileName, const std::string& str)
{
	if (fileName == "") fileName = GetNormalLogFileName();


	if (fileName.find("\\") == std::string::npos && fileName.find("/") == std::string::npos)
	{
		fileName = LOG_DIR + fileName;
	}




#ifdef _WIN32
	FILE* pFile = NULL;
	int err = fopen_s(&pFile, fileName.c_str(), "a+");	//追加文件记录
	if (err) return;

	fprintf(pFile, "%s", str.c_str());
	fclose(pFile);

#else
	FILE* pFile = NULL;
	try
	{
		pFile = fopen(fileName.c_str(), "a+");	//追加文件记录
		if (!pFile) return;

		fprintf(pFile, "%s", str.c_str());
		fclose(pFile);
	}
	catch (...)
	{
		if (pFile)fclose(pFile);
	}

#endif // _WIN32

}

std::string CLog::ReadFileToString(std::string filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);  // 以二进制模式打开并定位到文件末尾
	if (!file) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	std::streamsize size = file.tellg();  // 获取文件大小
	file.seekg(0, std::ios::beg);  // 移动回文件开头

	std::vector<char> buffer(size);  // 分配足够的缓冲区
	if (file.read(buffer.data(), size)) {
		return std::string(buffer.begin(), buffer.end());
	}
	throw std::runtime_error("Failed to read file: " + filename);



	//std::string back = "";
	//const int BUFFLEN = 5000;
	////建立文件输入流
	//std::locale loc = std::locale::global(std::locale(""));//要打开的文件路径含中文，设置全局locale为本地环境
	//std::ifstream ifs(fileName.c_str());
	//if (ifs)
	//{
	//	char buff[BUFFLEN];
	//	while (!ifs.eof())
	//	{
	//		ifs.getline(buff, BUFFLEN);
	//		back = back + std::string(buff) + "\r\n";
	//	}
	//}
	//std::locale::global(loc);//恢复全局locale
	//return back;

}

LogPtr CLog::Instance()
{
	if (!m_pLog)
	{
		create_directory(LOG_DIR);
		m_pLog = std::make_shared<CLog>();
	}
	return m_pLog;

}

void CLog::DelLogFile(const std::string& fileName)
{
	std::string filePath = fileName;
	if (fileName.find("\\") == std::string::npos && fileName.find("/") == std::string::npos)
	{
		filePath = LOG_DIR + fileName;
	}

	remove(filePath.c_str());
	return;

}

CLog::~CLog()
{
}
void CLog::PrintLog(std::string fileName, const char* format, ...)
{
#ifdef _WIN32
	std::lock_guard<std::mutex> lock(m_mutex);
	// 开始制作输出缓冲区
	int listCount = 0;
	va_list argList;

	va_start(argList, format);

	int  str_count = MY_VPRINTF(format, argList);

	// 打印数据超过缓冲区长度，则增加缓冲区的长度
	if (m_maxLogBuffLength <= str_count)
	{
		delete m_logBuff;
		m_maxLogBuffLength = str_count * 2;
		m_logBuff = new char[m_maxLogBuffLength];
	}

	ZeroMemory(m_logBuff, m_maxLogBuffLength);
	listCount += MY_VSNPRINTF(m_logBuff + listCount, m_maxLogBuffLength - listCount, format, argList);

	va_end(argList);

	if (listCount > (m_maxLogBuffLength - 1))
	{
		listCount = m_maxLogBuffLength - 1;
	}
	m_logBuff[m_maxLogBuffLength - 1] = '\0';

	// 开始输出到目标设备
	std::string timeStamp = CGlobal::GetTimeStr(time(NULL));
	CON_PRINTF("[%s] %s", timeStamp.c_str(), m_logBuff);
	if (fileName == "-1") return;

	if (fileName == "")
	{
		fileName = GetNormalLogFileName();
	}

	if (fileName.find("\\") == std::string::npos && fileName.find("/") == std::string::npos)
	{
		fileName = LOG_DIR + fileName;
	}


	// windows编译需要
	FILE* pFile;
	errno_t err = fopen_s(&pFile, fileName.c_str(), "a+");	//追加文件记录
	if (err) return;

	fprintf(pFile, "[%s] %s", timeStamp.c_str(), m_logBuff);
	fclose(pFile);
#else
	std::lock_guard<std::mutex> lock(m_mutex);

	va_list argList;
	va_start(argList, format);

	char buffer[600] = { 0 };
	vsprintf(buffer, format, argList);
	printf(buffer);

	va_end(argList);

	// linux需要
	if (fileName == "-1") return;
	if (fileName == "")
	{
		fileName = GetNormalLogFileName();
	}

	if (fileName.find("\\") == std::string::npos && fileName.find("/") == std::string::npos)
	{
		fileName = LOG_DIR + fileName;
	}


	FILE* pFile = NULL;
	try {
		pFile = fopen(fileName.c_str(), "a+");	//追加文件记录
		if (!pFile) return;

		string timeStamp = CGlobal::GetTimeStr(time(NULL));
		fprintf(pFile, "[%s] %s", timeStamp.c_str(), buffer);
		fclose(pFile);
	}
	catch (...) {
		if (pFile)fclose(pFile);
	}

#endif

}

std::string CLog::GetNormalLogFileName()
{
	std::string fileName = CGlobal::GetTimeStr(time(NULL));
	fileName = fileName.substr(0, 8);
	fileName += std::string(".log");
	return fileName;

}
