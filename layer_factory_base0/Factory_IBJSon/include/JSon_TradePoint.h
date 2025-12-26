#pragma once
#include "JSon.h"
#include <base_trade.h>
class CJSon_TradePoint : public CJSon
{
public:
	CJSon_TradePoint() { ; };
	virtual ~CJSon_TradePoint() { ; };


	virtual		void					Save_TradePoints(const TradePointPtrs& tradePoints) = 0;

	virtual		TradePointPtrs			Load_TradePoints() = 0;

};
typedef std::shared_ptr<CJSon_TradePoint> JSon_TradePointPtr;


