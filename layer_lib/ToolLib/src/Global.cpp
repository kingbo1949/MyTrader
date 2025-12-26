// #include "pch.h"
#include "Global.h"
#include <time.h>
#include <stdarg.h>
#include "Log.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>


#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/locale.hpp>
using namespace boost::algorithm;



//#include <boost/filesystem.hpp>
//using namespace boost::filesystem;



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






std::string CGlobal::GetTickTimeStr(time_t tickTime)
{
	// tickTime 以毫秒为单位
	int milliSecond = tickTime % 1000;
	std::string sencondPartStr = GetTimeStr(tickTime / 1000);
	return StringFormat("%s.%d", sencondPartStr.c_str(), milliSecond);
}

std::string CGlobal::GetTickRecTimeStr(time_t tickRecTime)
{
	// tickRecTime以纳秒为单位
	int nanoSecond = tickRecTime % 1000000000;
	std::string sencondPartStr = GetTimeStr(tickRecTime / 1000000000);
	return StringFormat("%s.%d", sencondPartStr.c_str(), nanoSecond);

}


std::string CGlobal::GetTimeStr(time_t sencondTime)
{
	struct tm localTime = {};

	// 获取本地时间（考虑夏令时）
#ifdef WIN32
	localtime_s(&localTime, &sencondTime); // Windows 使用 localtime_s
#else
	localtime_r(&timestamp, &localTime); // Linux/macOS 使用 localtime_r
#endif

	// 格式化时间为 YYYYMMDD hh:mm:ss
	std::ostringstream oss;
	oss << std::put_time(&localTime, "%Y%m%d %H:%M:%S");

	return oss.str();
}
time_t CGlobal::GetTimeByStr(const std::string& timeStr)
{
	std::tm tmTime = {};
	std::istringstream ss(timeStr);

	// 解析格式 YYYYMMDD hh:mm:ss
	ss >> std::get_time(&tmTime, "%Y%m%d %H:%M:%S");
	if (ss.fail()) {
		throw std::runtime_error("Invalid date format, expected YYYYMMDD hh:mm:ss");
	}

	// 让 `mktime` 自动处理夏令时
	tmTime.tm_isdst = -1;  // 让 `mktime` 自动判断是否适用夏令时
	return std::mktime(&tmTime);


}



time_t CGlobal::GetTickTimeByStr(const std::string& timeStr)
{
	std::vector<std::string> strs;
	split(strs, timeStr, is_any_of("."), token_compress_on);


	if (strs.size() == 2)
	{
		std::string yyyymmdd_hms = strs[0];
		std::string milliSecondStr = strs[1];
		return GetTimeByStr(yyyymmdd_hms) * 1000 + atoi(milliSecondStr.c_str());
	}
	if (strs.size() == 1)
	{
		std::string yyyymmdd_hms = strs[0];
		return GetTimeByStr(yyyymmdd_hms) * 1000;
	}
	exit(-1);
	return 0;
}

time_t CGlobal::QGetDayTime(time_t timeSecond)
{
	std::tm localTime{};
	if (localtime_s(&localTime, &timeSecond) != 0)
	{ // 安全版本
		throw std::runtime_error("Failed to get local time");
	}
	// 将时间设置为当日零点
	localTime.tm_hour = 0;
	localTime.tm_min = 0;
	localTime.tm_sec = 0;
	return std::mktime(&localTime); // 转换回时间戳

}

bool CGlobal::MyLocalTime(const time_t* pTime, struct tm* pTm)
{
	if (*pTime < 0 || NULL == pTm) return false;

#ifdef _WIN32
	// windows编译需要
	localtime_s(pTm, pTime);

#else
	// linux编译需要
	localtime_r(pTime, pTm);

#endif
	return  pTm->tm_mday >= 1 && pTm->tm_mday <= 31;

}

int CGlobal::SafePrintf(char* buff, int buffLength, const char* format, ...)
{
	// windows
	int listCount = 0;
	va_list argList;
	if (!buff) return 0;

	va_start(argList, format);

	// 测试缓冲区长度
#ifdef _WIN32
	int  str_count = MY_VPRINTF(format, argList);
	if (buffLength <= str_count)
	{
		// 缓冲区不够长，全部清掉返回
		ZeroMemory(buff, buffLength);
		return 0;
	}
	listCount += MY_VSNPRINTF(buff + listCount, buffLength - listCount, format, argList);

	if (listCount > (buffLength - 1))
	{
		listCount = buffLength - 1;
	}

	*(buff + listCount) = '\0';

#else
	vsprintf(buff, format, argList);
#endif
	va_end(argList);

	return listCount;

}




std::vector<std::string> CGlobal::SplitStr(const std::string& str, const std::string& tokens)
{
	std::vector<std::string> strs;
	split(strs, str, is_any_of(tokens), token_compress_on);
	return strs;

}

std::vector<std::string> CGlobal::Trim(const std::vector<std::string>& strs)
{
	std::vector<std::string> back;

	for (const auto& str : strs)
	{
		std::string temStr = str;
		trim(temStr);
		back.push_back(temStr);
	}
	return back;


}

std::string CGlobal::unicode_to_utf8(const std::string& match)
{
	// 提取码点（去掉 \u 前缀）
	std::string hex = match.substr(2);
	unsigned int codepoint = std::stoul(hex, nullptr, 16);

	// 将码点转换为 UTF-16（Boost 需要中间步骤）
	std::wstring utf16;
	if (codepoint <= 0xFFFF) {
		utf16.push_back(static_cast<wchar_t>(codepoint));
	}
	else {
		// 处理代理对（超出基本多文种平面，这里简化假设都在 BMP 内）
		utf16.push_back(static_cast<wchar_t>(codepoint));
	}

	// 使用 boost::locale 将 UTF-16 转换为 UTF-8
	return boost::locale::conv::utf_to_utf<char>(utf16);
}

std::string CGlobal::decode_unicode(const std::string& input)
{
	boost::regex re("\\\\u[0-9a-fA-F]{4}"); // 匹配 \uXXXX
	std::string result = input;

	boost::sregex_iterator it(input.begin(), input.end(), re);
	boost::sregex_iterator end;

	// 逐个替换
	std::string::size_type last_pos = 0;
	std::string temp;
	for (; it != end; ++it) {
		std::string match = it->str();
		std::string utf8_char = unicode_to_utf8(match);

		// 计算替换位置
		std::string::size_type pos = it->position();
		temp.append(result, last_pos, pos - last_pos);
		temp.append(utf8_char);
		last_pos = pos + match.length();
	}
	temp.append(result, last_pos, std::string::npos);

	return temp;
}

