#pragma once
#include "CheckMa.h"
class CCheckMa_Channel_Forward : public CCheckMa
{
public:
	CCheckMa_Channel_Forward(StrategyParamPtr pStrategyParam);
	virtual ~CCheckMa_Channel_Forward() { ; };

	virtual TrendType			GetTrendType() override final;

protected:

	// 得到当前K线是空头还是多头形态
	LongOrShort					GetKLineStatus();



};

