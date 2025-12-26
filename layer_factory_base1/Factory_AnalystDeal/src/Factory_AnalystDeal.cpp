#include "pch.h"
#include "Factory_AnalystDeal.h"

#include "ScanDealsForSimpleMatch_Real.h"
#include "AnalystSimpleMatch_Real.h"
#include "AnalystResultMonth_Real.h"

#include <Factory_IBGlobalShare.h>
#include <Global.h>

AnalystSimpleMatchPtr g_pAnalystSimpleMatch = nullptr;
FACTORY_ANALYSTDEAL_API AnalystSimpleMatchPtr MakeAndGet_AnalystSimpleMatchs()
{
	if (!g_pAnalystSimpleMatch)
	{
		g_pAnalystSimpleMatch = std::make_shared<CAnalystSimpleMatch_Real>();
	}
	return g_pAnalystSimpleMatch;
}


ScanDealsForSimpleMatchPtr g_pScanDealsForSimpleMatchs = nullptr;
FACTORY_ANALYSTDEAL_API ScanDealsForSimpleMatchPtr MakeAndGet_ScanDealsForSimpleMatchs()
{
	if (!g_pScanDealsForSimpleMatchs)
	{
		g_pScanDealsForSimpleMatchs = std::make_shared<CScanDealsForSimpleMatch_Real>();
	}
	return g_pScanDealsForSimpleMatchs;
}

FACTORY_ANALYSTDEAL_API AnalystResultMonthPtr MakeAndGet_AnalystSimpleMatchsMonth(const SimpleMatchPtrs& matchesMonth)
{
	return std::make_shared<CAnalystResultMonth_Real>(matchesMonth);
}

FACTORY_ANALYSTDEAL_API void SplitMatchsByMonth(const SimpleMatchPtrs& matches, std::map<Tick_T, SimpleMatchPtrs>& month_matches)
{
	for (const auto& onematch : matches)
	{
		Tick_T month = CGetRecordNo::GetRecordNo_Month(onematch->ordertime_open);
		std::map<Tick_T, SimpleMatchPtrs>::iterator pos = month_matches.find(month);
		if (pos != month_matches.end())
		{
			pos->second.push_back(onematch);
		}
		else
		{
			SimpleMatchPtrs matches;
			matches.push_back(onematch);
			month_matches[month] = matches;
		}
	}
	
}


