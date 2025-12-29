#pragma once
#include "Factory_AnalystDeal_Export.h"

#include "AnalystSimpleMatch.h"
#include "ScanDealsForSimpleMatch.h"
#include "AnalystResultMonth.h"


	FACTORY_ANALYSTDEAL_API AnalystSimpleMatchPtr				MakeAndGet_AnalystSimpleMatchs();

	FACTORY_ANALYSTDEAL_API ScanDealsForSimpleMatchPtr			MakeAndGet_ScanDealsForSimpleMatchs();

	FACTORY_ANALYSTDEAL_API AnalystResultMonthPtr				MakeAndGet_AnalystSimpleMatchsMonth(const SimpleMatchPtrs& matchesMonth);

	// 按月切分回合 也许这些回合来自不同的策略
	FACTORY_ANALYSTDEAL_API void								SplitMatchsByMonth(const SimpleMatchPtrs& matches, std::map<Tick_T, SimpleMatchPtrs>& month_matches);

