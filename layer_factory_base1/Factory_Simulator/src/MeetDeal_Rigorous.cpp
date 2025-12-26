#include "pch.h"
#include "MeetDeal_Rigorous.h"
#include <Factory_Log.h>
bool CMeetDeal_Rigorous::CanDeal(int priceStatus)
{
	// 盘口之外，不可成交
	if (priceStatus > 0) return false;

	if (priceStatus < 0)
	{
		// 对价吃 允许成交
		return true;
	}

	// 在盘口之内 不可成交
	if (priceStatus == 0)
	{
		return false;
	}

	return false;
}

double CMeetDeal_Rigorous::GetDealPrice(BuyOrSell buyOrSell, double priceIn, int priceStatus, IBTickPtr ptick, double minimove)
{
	if (priceStatus > 0)
	{
		Log_Print(LogLevel::Err, "can not GetDealPrice, since priceStatus > 0");
		exit(-1);
	}

	if (priceStatus == 0)
	{
		Log_Print(LogLevel::Err, "can not GetDealPrice, since priceStatus == 0");
		exit(-1);
	}
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
