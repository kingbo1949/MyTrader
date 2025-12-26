#pragma once
#include "JSon_TradePoint.h"
class CJSon_TradePoint_Real : public CJSon_TradePoint
{
public:
	CJSon_TradePoint_Real() { ; };
	virtual ~CJSon_TradePoint_Real() { ; };

	virtual		void					Save_TradePoints(const TradePointPtrs& tradePoints) override final;

	virtual		TradePointPtrs			Load_TradePoints() override final;

protected:

	std::string							GetFileName();


	Json::Value							MakeValue_OneTradePoint(const TradePointPtr& oneTradePoint);
	TradePointPtr						Make_OneTradePoint(const Json::Value& oneTradePointValue);

	Json::Value							MakeValue_TradePoints(const TradePointPtrs& tradePoints);
	TradePointPtrs						Make_TradePoints(const Json::Value& tradePointsValue);


};


