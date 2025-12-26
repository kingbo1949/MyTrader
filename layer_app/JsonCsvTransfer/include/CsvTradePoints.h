#pragma once
#include "CsvTransor.h"
#include <base_trade.h>
class CCsvTradePoints : public CCsvTransor
{
public:
	CCsvTradePoints() { ; };
	virtual ~CCsvTradePoints() { ; };

	// 从csv文件读取TradePointPtrs
	virtual void				LoadTradePointsFromCsv(const std::string& fileName, TradePointPtrs& tradePoints) = 0;

};
typedef std::shared_ptr<CCsvTradePoints> CsvTradePointsPtr;

