#pragma once
#include "AnalystDealDefine.h"

// 扫描deal.json文件形成simpleMatchs
class CScanDealsForSimpleMatch
{
public:
	CScanDealsForSimpleMatch() { ; };
	virtual ~CScanDealsForSimpleMatch() { ; };

	virtual std::map<StrategyId, SimpleMatchPtrs>			Make_SimpleMatchs_FromJson() = 0;
};
typedef std::shared_ptr<CScanDealsForSimpleMatch> ScanDealsForSimpleMatchPtr;


