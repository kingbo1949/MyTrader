#pragma once
#include "AnalystDealDefine.h"
class CAnalystSimpleMatch
{
public:
	CAnalystSimpleMatch() { ; };
	virtual ~CAnalystSimpleMatch() { ; };

	virtual AnalystResultPtr			Go(const SimpleMatchPtrs& matchs) = 0;

};
typedef std::shared_ptr<CAnalystSimpleMatch> AnalystSimpleMatchPtr;


