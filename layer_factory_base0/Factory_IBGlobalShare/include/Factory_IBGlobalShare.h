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
#include <MyThreadPool.h>
#include "CurrentTime.h"
#include "TransToStr.h"
#include "GetRecordNo.h"
#include "Trans.h"
#include "Reverse.h"
#include "HighFrequencyGlobalFunc.h"

#include "SeriesNoMaker.h"

#include "TransDataToStr.h"

#include <MyLogDefine.h>

// 运行时需要打印 但图形窗口程序与命令行程序的输出方式不同，因此要求应用层提供函数指针，以实现个性化输出
typedef void(*PFunc_Print)(LogLevel logLevel, const std::string& str);



#ifndef WIN32

#include "tscns.h"
extern TSCNS g_tn;

extern "C"
{

#endif // !WIN32

	FACTORY_IBGLOBALSHARE_API MyThreadPoolPtr					MakeAndGet_MyThreadPool();

	// Make_CurrentTime时，实盘状态microSecond设置为零， 模拟盘状态设置为第一个驱动数据之前1微妙
	FACTORY_IBGLOBALSHARE_API void								Make_CurrentTime(CurrentTimeType currentTimeType, MicroSecond_T microSecond);
	FACTORY_IBGLOBALSHARE_API CurrentTimePtr					Get_CurrentTime();
	FACTORY_IBGLOBALSHARE_API CurrentTimeType					Get_CurrentTimeType();

	FACTORY_IBGLOBALSHARE_API SeriesNoMakerPtr					MakeAndGet_SeriesNoMaker();

	// 个性化打印函数指针
	FACTORY_IBGLOBALSHARE_API void								Make_LogFunc(PFunc_Print pFunc_Print);
	FACTORY_IBGLOBALSHARE_API PFunc_Print						Get_LogFunc();
	FACTORY_IBGLOBALSHARE_API void								DefaultLogFunc(LogLevel logLevel, const std::string& str);


	class FACTORY_IBGLOBALSHARE_API  							CTransToStr;

	class FACTORY_IBGLOBALSHARE_API  							CTrans;

	class FACTORY_IBGLOBALSHARE_API  							CReverse;

	class FACTORY_IBGLOBALSHARE_API								CHighFrequencyGlobalFunc;

	class FACTORY_IBGLOBALSHARE_API								CTransDataToStr;

	class FACTORY_IBGLOBALSHARE_API								CGetRecordNo;








#ifndef WIN32
}
#endif // !WIN32




