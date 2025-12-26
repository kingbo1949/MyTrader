#pragma once
#include <string>
#include <time.h>
#include <vector>
#include <chrono>
#include <cmath>



// 用于打时间戳的内联函数
inline time_t benchmark_second()
{
	return time(nullptr);
}

#ifdef WIN32
// Windows版本
inline time_t benchmark_milliseconds()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline time_t benchmark_microsecond()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
inline time_t benchmark_nanoseconds()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


#else
// Linux版本 测试结果：调用一次大概10纳秒左右
#include <sys/time.h>
inline time_t benchmark_milliseconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

inline time_t benchmark_microsecond()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
	printf("microsecond: %ld\n", tv.tv_sec * 1000000 + tv.tv_usec); // 徽秒

}
inline time_t benchmark_nanoseconds()
{
	struct timespec tn;
	clock_gettime(CLOCK_REALTIME, &tn);
	return tn.tv_sec * 1000000000 + tn.tv_nsec;
}


#endif // WIN32




class CGlobal
{
public:
	CGlobal() { ; };
	virtual ~CGlobal() { ; };

	// 从毫秒时间返回字符串，格式为：[yyyymmdd hh:mm:ss.milliSecond]
	static std::string			GetTickTimeStr(time_t tickTime);

	// 从纳妙时间返回字符串，格式为：[yyyymmdd hh:mm:ss.nanoseconds]
	static std::string			GetTickRecTimeStr(time_t tickRecTime);

	//把一个时间按YYYYMMDD hh:mm:ss字符串返回
	static std::string			GetTimeStr(time_t sencondTime);


	//把一个YYYYMMDD hh:mm:ss格式的字符串返回时间数值（秒），是GetTimeStr的反函数
	static time_t				GetTimeByStr(const std::string& timeStr);

	//把一个YYYYMMDD hh:mm:ss.milliSecond格式的字符串返回时间数值（毫秒），是GetTickTimeStr的反函数
	static time_t				GetTickTimeByStr(const std::string& timeStr);

	static time_t				QGetDayTime(time_t timeSecond);						//把一个时间转换成当天的零点


	// 解决localtime函数windows和linux兼容问题
	static bool					MyLocalTime(const time_t* pTime, struct tm* pTm);

	// 平台兼容并且安全地格式化字符串，返回值为字符总数，相当于strlen,不包括最后的‘\0’
	// 返回0或者负值表示出错，一般都是因为缓冲区设得太小
	static int                 SafePrintf(char* buff, int buffLength, const char* format, ...);

	// 把double转换为int,避免强制转换时损失精度
	inline static int			DToI(double d)
	{
		return (d >= 0) ? (int)(d + 0.5) : (int)(d - 0.5);
	}

	// 检查浮点数是否是零
	inline static int			IsZero(double d)
	{
		if (abs(d) < 0.00001) return true;
		return false;
	}

	// 浮点数是否相等
	inline static int			IsEqual(double d1, double d2)
	{
		return IsZero(d1 - d2);
	}

	// 切分字符串
	static std::vector<std::string>	SplitStr(const std::string& str, const std::string& tokens);

	// 把strs中的前后空格清除后返回
	static std::vector<std::string>		Trim(const std::vector<std::string>& strs);

	template<typename... Args>
	static std::string				StringFormat(const char* sformat, Args&& ... args)
	{
		char buf[512] = { 0 };
		snprintf(buf, sizeof(buf), sformat, std::forward<Args>(args)...);
		return buf;
	}

	// 将单个 \uXXXX 转换为 UTF-8
	static std::string unicode_to_utf8(const std::string& match);

	// 解码整个字符串
	static std::string decode_unicode(const std::string& input);
};

