#include "pch.h"
#include "MeetDeal_Loose.h"
#include <Factory_Log.h>

bool CMeetDeal_Loose::CanDeal(int priceStatus)
{
	// 盘口之外，不可成交
	if (priceStatus > 1) return false;

	// 盘口之内，可成交
	if (priceStatus == 0) return true;

	// 压在盘口之上，可成交
	if (priceStatus == 1) return true;

	// 过价可成交
	return true;

}

double CMeetDeal_Loose::GetDealPrice(BuyOrSell buyOrSell, double priceIn, int priceStatus, IBTickPtr ptick, double minimove)
{
	if (priceStatus > 1)
	{
		Log_Print(LogLevel::Err, "can not GetDealPrice, since priceStatus > 1");
		exit(-1);
	}

	if (priceStatus == 1)
	{
		// 压在盘口上
		return priceIn;
	}

	if (priceStatus == 0)
	{
		// 盘口之内
		return priceIn;
	}

	// 过价成交
	double bid = ptick->bidAsks[0].bid * minimove;
	double ask = ptick->bidAsks[0].ask * minimove;
	if (buyOrSell == BuyOrSell::Buy)
	{
		return std::max(ask, priceIn);
	}
	else
	{
		return std::min(bid, priceIn);
	}
}
