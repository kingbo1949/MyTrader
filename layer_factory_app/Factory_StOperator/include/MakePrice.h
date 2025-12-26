#pragma once

#include <base_trade.h>
class CMakePrice
{
public:
	CMakePrice() { ; };
	virtual ~CMakePrice() { ; };

	// 返回下单的价格， 
	virtual bool			GetPrice(double& price, RealPriceType& priceType) = 0;

protected:

	// 临近收盘是否可以发单，缺省实现是允许平仓不允许开仓
	virtual bool			CanMakePriceNearClose() = 0;



};
typedef std::shared_ptr<CMakePrice> MakePricePtr;

