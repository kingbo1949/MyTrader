// #include "pch.h"
#include "Log.h"
#include <fstream>
#include <stdarg.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>
using namespace boost::filesystem;


#include "Global.h"

#if defined(_WIN32)
#include <windows.h>
#endif







#define LOG_DIR "log/"

CLog::CLog()
	:m_maxLogBuffLength(0), m_logBuff(nullptr)
{

}

LogPtr CLog::m_pLog = NULL;

void CLog::DebugOutput(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	std::string msg = vformat_string(format, args);
	va_end(args);

#if defined(_WIN32)
	// Windows：调试输出
#ifdef _DEBUG
	OutputDebugStringA(msg.c_str());
#else
	std::fprintf(stdout, "%s", msg.c_str());
#endif
#else
	// macOS：直接输出到 stdout
	std::fprintf(stdout, "%s", msg.c_str());
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

std::string CLog::vformat_string(const char *fmt, va_list args)
{
	if (!fmt) return {};

	va_list args_copy;
	va_copy(args_copy, args);

	int required = 0;
#if defined(_WIN32)
	required = _vscprintf(fmt, args_copy);
#else
	required = vsnprintf(nullptr, 0, fmt, args_copy);
#endif
	va_end(args_copy);

	if (required < 0) return {};

	std::vector<char> buf(static_cast<size_t>(required) + 1);

#if defined(_WIN32)
	vsnprintf_s(buf.data(), buf.size(), _TRUNCATE, fmt, args);
#else
	vsnprintf(buf.data(), buf.size(), fmt, args);
#endif

	buf.back() = '\0';
	return std::string(buf.data());
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
	std::lock_guard<std::mutex> lock(m_mutex);

	va_list args;
	va_start(args, format);
	std::string msg = vformat_string(format, args);
	va_end(args);

	std::string timeStamp = CGlobal::GetTimeStr(time(NULL));

	// 输出到控制台（避免 format-string 风险）
	std::fprintf(stdout, "[%s] %s", timeStamp.c_str(), msg.c_str());

	if (fileName == "-1") return;

	if (fileName.empty())
		fileName = GetNormalLogFileName();

	if (fileName.find("\\") == std::string::npos && fileName.find("/") == std::string::npos)
		fileName = std::string(LOG_DIR) + fileName;

#if defined(_WIN32)
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, fileName.c_str(), "a+");
	if (err || !pFile) return;
#else
	FILE* pFile = std::fopen(fileName.c_str(), "a+");
	if (!pFile) return;
#endif

	std::fprintf(pFile, "[%s] %s", timeStamp.c_str(), msg.c_str());
	std::fclose(pFile);

}

std::string CLog::GetNormalLogFileName()
{
	std::string fileName = CGlobal::GetTimeStr(time(NULL));
	fileName = fileName.substr(0, 8);
	fileName += std::string(".log");
	return fileName;

}
