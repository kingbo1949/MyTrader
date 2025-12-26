#include "pch.h"
#include "Reverse.h"

BuyOrSell CReverse::Reverse_BuyOrSell(BuyOrSell buyOrSell)
{
	if (buyOrSell == BuyOrSell::Buy)
	{
		return BuyOrSell::Sell;
	}
	else
	{
		return BuyOrSell::Buy;
	}

}

RoundAction CReverse::Reverse_OpenOrCover(RoundAction openOrCover)
{
	if (openOrCover == RoundAction::Open_Round)
	{
		return RoundAction::Cover_Round;
	}
	else
	{
		return RoundAction::Open_Round;
	}
}
