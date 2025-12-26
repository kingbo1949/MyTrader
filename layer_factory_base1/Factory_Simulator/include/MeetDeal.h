#pragma once

#include <base_struc.h>
#include <base_trade.h>
class CMeetDeal
{
public:
	CMeetDeal() { ; };
	virtual ~CMeetDeal() { ; };

	// 按价位形态判断是否可以成交
	virtual bool			CanDeal(int priceStatus) = 0;

	// 得到成交价格
	virtual double			GetDealPrice(BuyOrSell buyOrSell, double priceIn, int priceStatus, IBTickPtr ptick, double minimove) = 0;


};
typedef std::shared_ptr<CMeetDeal> MeetDealPtr;
