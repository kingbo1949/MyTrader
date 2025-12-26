#include "pch.h"
#include "CheckMa_Channel_Forward.h"

CCheckMa_Channel_Forward::CCheckMa_Channel_Forward(StrategyParamPtr pStrategyParam)
	:CCheckMa(pStrategyParam)
{
}


TrendType CCheckMa_Channel_Forward::GetTrendType()
{

	LongOrShort longOrShort = GetKLineStatus();

	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		if (longOrShort == LongOrShort::LongT)
		{
			return TrendType::ProTrend;
		}
	}
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Sell)
	{
		if (longOrShort == LongOrShort::ShortT)
		{
			return TrendType::ProTrend;
		}
	}

	return TrendType::AntiTrend;
}

LongOrShort CCheckMa_Channel_Forward::GetKLineStatus()
{
	LongOrShort longOrShort = LongOrShort::LongT;
	if (m_pMaResult->klines_d1.back()->close >= m_pMaResult->ma_d1_5)
	{
		longOrShort = LongOrShort::LongT;	// 当下均线多头形态
	}
	else
	{
		longOrShort = LongOrShort::ShortT;	// 当下均线空头形态
	}


	//LongOrShort longOrShort = LongOrShort::LongT;
	//if (m_pMaResult->ma_h1_20 >= m_pMaResult->ma_h1_60)
	//{
	//	longOrShort = LongOrShort::LongT;	// 当下均线多头形态
	//}
	//else
	//{
	//	longOrShort = LongOrShort::ShortT;	// 当下均线空头形态
	//}

	return longOrShort;
}
