#include "TSideTimeHandler.h"

TSideRanges CTSideTimeHandler::ScanKlines(const IBKLinePtrs& klines)
{
	TSideRanges ret;

	TSideRange oneRange;
	for (auto i = 0; i < klines.size(); ++i)
	{
		TradeSideType type = GetTSideType(klines[i]->time);
		if (oneRange.beginIndex == -1)
		{
			// 初始状态
			oneRange.beginIndex = i;
			oneRange.endIndex = i;
			oneRange.tSideType = type;
		}
		else
		{
			if (oneRange.tSideType == type)
			{
				// 状态继续
				oneRange.endIndex = i;
			}
			else
			{
				// 状态结束
				ret.push_back(oneRange);

				// 开始新状态
				oneRange.beginIndex = i;
				oneRange.endIndex = i;
				oneRange.tSideType = type;
			}
		}
	}
	if (oneRange.beginIndex != -1)
	{
		ret.push_back(oneRange);
	}
	return ret;
}
