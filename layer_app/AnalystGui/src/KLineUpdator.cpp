#include "KLineUpdator.h"
#include <Global.h>
CKLineUpdator::CKLineUpdator()
{
}

int CKLineUpdator::Update(const IBKLinePtrs& klines, KlinePlotSuit& suit)
{
	int ret = 0;
	for (int i = 0; i < klines.size(); ++i)
	{
		IBKLinePtr kline = klines[i];
		Time_Index::const_iterator p = suit.time_index.find(kline->time);
		if (p == suit.time_index.end())
		{
			// 开始添加
			AddToSuit(kline, suit);
			++ret;
		}
		else
		{
			// 开始更新
			UpdateToSuit(kline, p->second, suit);
		}
	}
	return ret;
}

void CKLineUpdator::AddToSuit(IBKLinePtr kline, KlinePlotSuit& suit)
{
	suit.time_index[kline->time] = int(suit.klines.size());
	suit.klines.push_back(kline);
	return;
}

void CKLineUpdator::UpdateToSuit(IBKLinePtr kline, int pos, KlinePlotSuit& suit)
{
	suit.klines[pos] = kline;
	return;

}
