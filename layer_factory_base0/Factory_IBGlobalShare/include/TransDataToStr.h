#pragma once

#ifdef WIN32

#ifdef FACTORY_IBGLOBALSHARE_EXPORTS
#define FACTORY_IBGLOBALSHARE_API __declspec(dllexport)
#else
#define FACTORY_IBGLOBALSHARE_API __declspec(dllimport)
#endif


#else

#define FACTORY_IBGLOBALSHARE_API 

#endif // WIN32

#include <base_struc.h>

class FACTORY_IBGLOBALSHARE_API CTransDataToStr
{
public:
	CTransDataToStr() { ; };
	virtual ~CTransDataToStr() { ; };

	// tick转换到字符串(用于打印log文件，不含字段名称, bidask组成单一字符串不含逗号隔开)
	static std::string						GetStr_Print(IBTickPtr thisTick, double minimove);

	// tick转换到字符串(用于打印csv文件，不含字段名称，bidask含逗号隔开)
	static std::string						GetStr_Csv(IBTickPtr thisTick, double minimove);

	// tick转换到字符串(用于打印csv文件，不含字段名称，bidask含逗号隔开)
	static std::string						GetTickDepthStr_Csv(IBTickPtr thisTick, double minimove);

	// kline转换到字符串(用于打印csv文件，不含字段名称)
	static std::string						GetKlineStr_Csv(IBKLinePtr kline);
	static IBKLinePtr						GetKlineStr_Csv(const std::string& str);

	// ma转换到字符串(用于打印csv文件，不含字段名称)
	static std::string						GetMaStr_Csv(IBMaPtr maline);

	// K线输出到CSV文件时的title
	static std::string						GetKlineTitle_Csv();

	// 1档tick线输出到CSV文件时的title
	static std::string						GetTickTitle_Csv();

	// 5档tick线输出到CSV文件时的title
	static std::string						GetDeepTickTitle_Csv();
};

