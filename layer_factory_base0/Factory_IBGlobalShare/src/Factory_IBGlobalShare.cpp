#include "pch.h"
#include "Factory_IBGlobalShare.h"
#include "CurrentTime_RunTime.h"
#include "CurrentTime_Simulation.h"
#include <Factory_Log.h>
#include <Log.h>
#include "SeriesNoMaker_Real.h"


MyThreadPoolPtr g_pThreadPool = NULL;
FACTORY_IBGLOBALSHARE_API MyThreadPoolPtr MakeAndGet_MyThreadPool()
{
	if (!g_pThreadPool)
	{
		const int threadSize = 8;
		g_pThreadPool = std::make_shared<CMyThreadPool>(threadSize);
	}
	return g_pThreadPool;
}


#ifndef WIN32
TSCNS g_tn;

#endif // !WIN32


CurrentTimePtr	g_pCurrentTime = nullptr;
CurrentTimeType g_currentTimeType = CurrentTimeType::For_Real;
FACTORY_IBGLOBALSHARE_API void Make_CurrentTime(CurrentTimeType currentTimeType, MicroSecond_T microSecond)
{
#ifndef WIN32
	// 初始化TSCNS
	g_tn.init();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	g_tn.calibrate();

#endif // !WIN32

	g_currentTimeType = currentTimeType;

	if (g_pCurrentTime) return;

	if (currentTimeType == CurrentTimeType::For_Real)
	{
		g_pCurrentTime = std::make_shared<CCurrentTime_RunTime>(microSecond);
	}
	else
	{
		g_pCurrentTime = std::make_shared<CCurrentTime_Simulation>(microSecond);
	}
	return;

}

FACTORY_IBGLOBALSHARE_API CurrentTimePtr Get_CurrentTime()
{
	if (!g_pCurrentTime)
	{
		Log_Print(LogLevel::Err, "m_pCurrentTime is null");
		exit(1);
	}
	return g_pCurrentTime;

}

FACTORY_IBGLOBALSHARE_API CurrentTimeType Get_CurrentTimeType()
{
	return g_currentTimeType;
}

SeriesNoMakerPtr m_pSeriesNoMaker = NULL;
FACTORY_IBGLOBALSHARE_API SeriesNoMakerPtr MakeAndGet_SeriesNoMaker()
{
	if (!m_pSeriesNoMaker)
	{
		m_pSeriesNoMaker = std::make_shared<CSeriesNoMaker_Real>();
	}

	return m_pSeriesNoMaker;

}
PFunc_Print g_pFunc_Print = DefaultLogFunc;
FACTORY_IBGLOBALSHARE_API void Make_LogFunc(PFunc_Print pFunc_Print)
{
	g_pFunc_Print = pFunc_Print;
	return;
}

FACTORY_IBGLOBALSHARE_API PFunc_Print Get_LogFunc()
{
	if (!g_pFunc_Print)
	{
		printf("g_pFunc_Print is null \n");
		exit(-1);
	}
	return g_pFunc_Print;
}

FACTORY_IBGLOBALSHARE_API void DefaultLogFunc(LogLevel logLevel, const std::string& str)
{
	Log_Print(logLevel, str);
	if (logLevel == LogLevel::Critical)
	{
		// 严重错误打印到文件
		CLog::Instance()->PrintStrToFile("error.log", str);
	}
	return;
}

