#pragma once



#ifdef WIN32

#ifdef FACTORY_MONITOR_EXPORTS
#define FACTORY_MONITOR_API __declspec(dllexport)
#else
#define FACTORY_MONITOR_API __declspec(dllimport)
#endif


#else

#define FACTORY_MONITOR_API 

#endif // WIN32

#include "Monitor.h"
#ifndef WIN32
extern "C"
{
#endif // !WIN32

	FACTORY_MONITOR_API MonitorPtr				MakeAndGet_Monitor();


#ifndef WIN32
}
#endif // !WIN32
