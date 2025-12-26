#pragma once
#include "CsvTradePoints.h"
class CCsvTradePoints_Real : public CCsvTradePoints
{
public:
	CCsvTradePoints_Real() { ; };
	virtual ~CCsvTradePoints_Real() { ; };

	// 从csv文件读取TradePointPtrs
	virtual void				LoadTradePointsFromCsv(const std::string& fileName, TradePointPtrs& tradePoints) override final;

protected:
	// csv字符串转换成结构
	TradePointPtr				MakeTradePoint(const std::string& csvStr);


};

