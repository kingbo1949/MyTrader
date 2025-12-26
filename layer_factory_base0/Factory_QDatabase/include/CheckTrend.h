#pragma once
#include <base_struc.h>
#include <base_trade.h>
// 用均线检查顺势还是逆势
class CCheckTrend
{
public:
	CCheckTrend() { ; };
	virtual ~CCheckTrend() { ; };

	// 查看指定价格的操作是顺势还是逆势
	virtual bool		GetTrendType(double price, LongOrShort wantoDo, TrendType& trendType) = 0;


};
typedef std::shared_ptr<CCheckTrend> CheckTrendPtr;
