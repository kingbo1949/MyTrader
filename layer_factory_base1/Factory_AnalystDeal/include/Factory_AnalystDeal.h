#pragma once

#ifdef WIN32

#ifdef FACTORY_ANALYSTDEAL_EXPORTS
#define FACTORY_ANALYSTDEAL_API __declspec(dllexport)
#else
#define FACTORY_ANALYSTDEAL_API __declspec(dllimport)
#endif


#else

#define FACTORY_ANALYSTDEAL_API 

#endif // WIN32

#include "AnalystSimpleMatch.h"
#include "ScanDealsForSimpleMatch.h"
#include "AnalystResultMonth.h"

#ifndef WIN32
extern "C"
{
#endif // !WIN32

	FACTORY_ANALYSTDEAL_API AnalystSimpleMatchPtr				MakeAndGet_AnalystSimpleMatchs();

	FACTORY_ANALYSTDEAL_API ScanDealsForSimpleMatchPtr			MakeAndGet_ScanDealsForSimpleMatchs();

	FACTORY_ANALYSTDEAL_API AnalystResultMonthPtr				MakeAndGet_AnalystSimpleMatchsMonth(const SimpleMatchPtrs& matchesMonth);

	// 按月切分回合 也许这些回合来自不同的策略
	FACTORY_ANALYSTDEAL_API void								SplitMatchsByMonth(const SimpleMatchPtrs& matches, std::map<Tick_T, SimpleMatchPtrs>& month_matches);

#ifndef WIN32
}
#endif // !WIN32
