#pragma once


#ifdef WIN32

#ifdef FACTORY_SIMULATOR_EXPORTS
#define FACTORY_SIMULATOR_API __declspec(dllexport)
#else
#define FACTORY_SIMULATOR_API __declspec(dllimport)
#endif


#else

#define FACTORY_SIMULATOR_API 

#endif // WIN32

#include "Simulator.h"
#include "MeetDeal.h"

#ifndef WIN32
extern "C"
{
#endif // __cplusplus


	FACTORY_SIMULATOR_API SimulatorPtr					MakeAndGet_Simulator();

	FACTORY_SIMULATOR_API MeetDealPtr					MakeAndGet_MeetDeal();




#ifndef WIN32
}
#endif // __cplusplus





