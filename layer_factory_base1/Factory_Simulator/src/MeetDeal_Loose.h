#pragma once
#include "MeetDeal.h"
class CMeetDeal_Loose : public CMeetDeal
{
public:
	CMeetDeal_Loose() { ; };
	virtual ~CMeetDeal_Loose() { ; };

	// 按价位形态判断是否可以成交
	virtual bool			CanDeal(int priceStatus) override final;

	// 得到成交价格
	virtual double			GetDealPrice(BuyOrSell buyOrSell, double priceIn, int priceStatus, IBTickPtr ptick, double minimove)  override final;

};

