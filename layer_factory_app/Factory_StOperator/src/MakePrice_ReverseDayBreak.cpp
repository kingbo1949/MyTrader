#include "pch.h"
#include "MakePrice_ReverseDayBreak.h"

CMakePrice_ReverseDayBreak::CMakePrice_ReverseDayBreak(const SubModuleParams& stParams)
	:CMakePrice_DayBreak(stParams)
{
}

bool CMakePrice_ReverseDayBreak::GetOpenPrice(double& price, RealPriceType& priceType)
{
	return CMakePrice_DayBreak::GetOpenPrice(price, priceType);
}

bool CMakePrice_ReverseDayBreak::GetCoverPrice(double& price, RealPriceType& priceType)
{
	return CMakePrice_DayBreak::GetCoverPrice(price, priceType);
}
