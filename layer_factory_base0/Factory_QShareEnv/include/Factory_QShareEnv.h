#pragma once


#ifdef WIN32

#ifdef FACTORY_QSHAREENV_EXPORTS
#define FACTORY_QSHAREENV_API __declspec(dllexport)
#else
#define FACTORY_QSHAREENV_API __declspec(dllimport)
#endif


#else

#define FACTORY_QSHAREENV_API 

#endif // WIN32

#include "QEnvManager.h"

#ifndef WIN32
extern "C"
{
#endif // !WIN32


	FACTORY_QSHAREENV_API QEnvManagerPtr					MakeAndGet_QEnvManager();


#ifndef WIN32
}
#endif // !WIN32






