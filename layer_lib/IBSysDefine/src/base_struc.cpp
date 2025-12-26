

#include "pch.h"
#include "base_struc.h"



IBTickPtr CIBTick::Clone()
{
	IBTickPtr back = std::make_shared<CIBTick>();

	back->codeHash = codeHash;
	back->time = time;
	copy(bidAsks.begin(), bidAsks.end(), back_inserter(back->bidAsks));

	back->open = open;
	back->last = last;
	back->vol = vol;

	back->totalVol = totalVol;
	back->turnOver = turnOver;
	back->timeStamp = timeStamp;


	return back;


}

IbContractPtr CIbContract::Clone()
{
	IbContractPtr back = std::make_shared<CIbContract>();
	back->codeId = codeId;
	back->localSymbol = localSymbol;
	back->securityType = securityType;

	back->exchangePl = exchangePl;
	back->primaryExchangePl = primaryExchangePl;

	back->currencyID = currencyID;
	back->decDigits = decDigits;
	back->minMove = minMove;
	back->enable = enable;


	return back;


}

IBKLinePtr CIBKLine::Clone()
{
	IBKLinePtr back = std::make_shared<CIBKLine>();
	back->time = time;
	back->open = open;
	back->close = close;
	back->high = high;
	back->low = low;
	back->vol = vol;
	return back;
}

double& g_GetCircleValue(AverageValue& value, int circle)
{
	if (circle == 5) return value.v5;
	if (circle == 20) return value.v20;
	if (circle == 60) return value.v60;
	if (circle == 200) return value.v200;

	printf("error circle, circle = %d \n", circle);
	exit(-1);
	return value.v5;

}
double g_SumClose(double acc, const IBKLinePtr kline)
{
	return acc + kline->close;
}