#include "pch.h"
#include "MakePrice_DoubleBreak.h"
#include <Factory_UnifyInterface.h>
#include <Global.h>
#include <GetRecordNo.h>
#include <Factory_TShareEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Log.h>
#include <Factory_IBJSon.h>
#include <Factory_Log.h>
#include "KLineAndMaTool.h"
CMakePrice_DoubleBreak::CMakePrice_DoubleBreak(const SubModuleParams& stParams)
	:CMakePrice_Real(stParams)
{
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		m_klinecount_open = 5;
		m_klinecount_cover = 1;
		m_maxExternCount = 1;
		m_maxHighLowDiff = 100.0;
		//m_maxHighLowDiff = 120.0;

	}
	else
	{
		m_klinecount_open = 3;
		m_klinecount_cover = 1;
		m_maxExternCount = 3;
		m_maxHighLowDiff = 120.0;

	}


	m_externPsCount = 0;
}

bool CMakePrice_DoubleBreak::GetOpenPrice(double& price, RealPriceType& priceType)
{

	if (Get_Sys_Status() != Thread_Status::Running) return false;

	if (!IsRegularTradeTime()) return false;
	

	time_t lastOpenTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));
	lastOpenTime = CGetRecordNo::GetRecordNo(Time_Type::M1, lastOpenTime);
	time_t lastCoverTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubCover));
	lastCoverTime = CGetRecordNo::GetRecordNo(Time_Type::M1, lastCoverTime);

	if (NowM1() == lastOpenTime)
	{
		// 重复开仓
		return false;
	}
	if (NowM1() == lastCoverTime)
	{
		// 本bar已经平过仓不要再开仓
		return false;
	}

	std::string timestr1 = CGlobal::GetTickTimeStr(Now());
	std::string timestrLastOpen = CGlobal::GetTickTimeStr(lastOpenTime);
	std::string timestrLastCover = CGlobal::GetTickTimeStr(lastCoverTime);

	double highLowDiff = 0.0;
	bool back = CheckOpenPrice(price, priceType, highLowDiff);
	if (back && highLowDiff < m_maxHighLowDiff) return true;

	return false;



}

bool CMakePrice_DoubleBreak::GetCoverPrice(double& price, RealPriceType& priceType)
{
	time_t lastOpenTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));
	time_t lastCoverTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubCover));

	std::string timestr1 = CGlobal::GetTickTimeStr(Now());
	std::string timestrLastOpen = CGlobal::GetTickTimeStr(lastOpenTime);
	std::string timestrLastCover = CGlobal::GetTickTimeStr(lastCoverTime);
	if (CheckPs() && m_externPsCount < m_maxExternCount)
	{
		PositionSizePtr ps = MakeAndGet_DbPositionSize()->GetOne(m_stParams.strategyIdHashId);
		if (ps)
		{
			ps->openTickTime = Now();
			++m_externPsCount;
			Log_Print(LogLevel::Info, "extern position");
		}
	}

	bool back = GetCoverPriceByTime(price, priceType);
	if (back)
	{
		m_externPsCount = 0;
	}
	return back;

}

bool CMakePrice_DoubleBreak::GetCoverPriceByBreak(double& price, RealPriceType& priceType)
{
	std::string timestr = CGlobal::GetTickTimeStr(Now());
	// 平仓逻辑与开仓完全相同
	priceType = RealPriceType::Eat;

	// 得到开仓单成交时间
	//time_t lastOpenDealTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));
	//if (CGetRecordNo::GetRecordNo(GetTimeType(), lastOpenDealTime) == CGetRecordNo::GetRecordNo(GetTimeType(), Now()))
	//{
	//	// 仍然处于开仓BAR
	//	return false;
	//}
	//time_t nextOpenTime = CGetRecordNo::GetRecordNoEnd(GetTimeType(), lastOpenDealTime);
	//string nextOpenTimetimestr = CGlobal::GetTickTimeStr(nextOpenTime);
	//HighAndLow highAndLow = CKLineAndMaTool::MakeHighAndLow(GetCodeId(), GetTimeType(), nextOpenTime, m_klinecount_cover, true);

	HighAndLow highAndLow = CKLineAndMaTool::MakeHighAndLow(GetCodeId(), GetTimeType(), Now(), m_klinecount_cover, true);

	// 检查1分钟线上是否仍然是突破状态
	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (!thiskline)
	{
		return false;
	}
	if (m_stParams.buyOrSell == BuyOrSell::Buy && thiskline->high > highAndLow.high )
	{
		price = std::max(highAndLow.high, thiskline->open);
		return true;
	}
	if (m_stParams.buyOrSell == BuyOrSell::Sell && thiskline->low < highAndLow.low )
	{
		price = std::min(highAndLow.low, thiskline->open);
		return true;
	}
	return false;
}

bool CMakePrice_DoubleBreak::GetCoverPriceByTime(double& price, RealPriceType& priceType)
{
	std::string timestr = CGlobal::GetTickTimeStr(Now());

	priceType = RealPriceType::Eat;
	int count = 1;
	if (IsCanCoverTime(count))
	{
		// 已到平仓时间
		IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
		if (!thiskline) return false;

		price = thiskline->open;
		return true;
	}
	// 判断是否需要止损
	if (GetCoverPriceByBreak(price, priceType))
	{
		return true;
	}

	//if (GetCoverPriceByStopLoss(price, priceType))
	//{
	//	return true;
	//}

	return false;
}

bool CMakePrice_DoubleBreak::IsCanCoverTime(int count)
{
	PositionSizePtr ps = MakeAndGet_DbPositionSize()->GetOne(m_stParams.strategyIdHashId);
	if (!ps)
	{
		Log_Print(LogLevel::Err, fmt::format("want to cover, ps is null, {}", m_pStrategyParam->key.ToStr().c_str()));
		exit(-1);
	}
	// 
	time_t endtime = CGetRecordNo::GetRecordNo(GetTimeType(), ps->openTickTime) + (count + 1) * CGetRecordNo::GetTimeSpan(GetTimeType());
	std::string timestr1 = CGlobal::GetTickTimeStr(endtime);
	//printf("endtime is %s \n", timestr1.c_str());
	std::string timestr2 = CGlobal::GetTickTimeStr(Now());
	if (Now() < endtime) return false;
	return true;
}

bool CMakePrice_DoubleBreak::IsRegularTradeTime()
{
	// 温哥华05:00:00 - 13:50:00
	// 美东08:00:00 - 17:50:00

	// 零点毫秒
	time_t daytime = CHighFrequencyGlobalFunc::GetDayMillisec();
	//time_t beginPos = daytime + 8 * 60 * 60 * 1000;
	//time_t endPos = daytime + 17 * 60 * 60 * 1000  + 50 * 60 * 1000;

	time_t beginPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("09:30:00", 0);
	time_t endPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("15:30:00", 0);


	if (Now() < beginPos || Now() > endPos) return false;
	return true;

}

bool CMakePrice_DoubleBreak::GetCoverPriceByStopLoss(double& price, RealPriceType& priceType)
{
	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (!thiskline) return false;

	double openprice;
	if (!GetOpenPrice(openprice)) return false;


	priceType = RealPriceType::Eat;
	double stoploss = 20;
	if (m_externPsCount != 0)
	{
		stoploss = 40;
	}
	if (m_stParams.buyOrSell == BuyOrSell::Buy && thiskline->high > openprice + stoploss)
	{
		price = std::max(openprice + stoploss, thiskline->open);
		return true;
	}
	if (m_stParams.buyOrSell == BuyOrSell::Sell && thiskline->low < openprice - stoploss)
	{
		price = std::min(openprice - stoploss, thiskline->open);
		return true;
	}
	return false;



}

bool CMakePrice_DoubleBreak::GetOpenPrice(double& price)
{
	PositionSizePtr ps = MakeAndGet_DbPositionSize()->GetOne(m_stParams.strategyIdHashId);
	if (!ps) return false;

	IBKLinePtr pskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, ps->openTickTime);
	if (!pskline) return false;


	HighAndLow highAndLow = CKLineAndMaTool::MakeHighAndLow(GetCodeId(), GetTimeType(), ps->openTickTime, m_klinecount_open, false);
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		price = std::max(highAndLow.high, pskline->open);
	}
	else
	{
		price = std::min(highAndLow.low, pskline->open);
	}
	return true;

}

bool CMakePrice_DoubleBreak::CheckPs()
{
	PositionSizePtr ps = MakeAndGet_DbPositionSize()->GetOne(m_stParams.strategyIdHashId);
	if (!ps) return false;

	// 检查当前BAR是否已经扩展过持仓
	if (CGetRecordNo::GetRecordNo(GetTimeType(), Now()) == CGetRecordNo::GetRecordNo(GetTimeType(), ps->openTickTime)) return false;

	double price = 0.0;
	RealPriceType priceType = RealPriceType::Eat;

	double highLowDiff = 0.0;
	//return CheckOpenPrice(price, priceType, highLowDiff);

	return MakeGreatBreak();

}

bool CMakePrice_DoubleBreak::CheckOpenPrice(double& price, RealPriceType& priceType, double& highLowDiff)
{
	// 当前BAR的末尾N分钟才可以开仓
	int n = 3;
	if (CGetRecordNo::GetRecordNoEnd(GetTimeType(), Now()) - n * 60 * 1000 > Now()) return false;

	std::string timestr = CGlobal::GetTickTimeStr(Now());

	HighAndLow highAndLow = CKLineAndMaTool::MakeHighAndLow(GetCodeId(), GetTimeType(), Now(), m_klinecount_open, false);
	highLowDiff = highAndLow.high - highAndLow.low;

	//IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), GetTimeType(), Now());
	//if (!thiskline) return false;
	//highLowDiff = thiskline->high - highAndLow.low;


	// 检查1分钟线上是否仍然是突破状态
	IBKLinePtr thisklineM1 = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (!thisklineM1) return false;

	priceType = RealPriceType::Eat;
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy && thisklineM1->high > highAndLow.high )
	{
		price = std::max(thisklineM1->open, highAndLow.high);
		return true;
	}
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Sell && thisklineM1->low < highAndLow.low )
	{
		price = std::min(thisklineM1->open, highAndLow.low);
		return true;
	}
	return false;
}

bool CMakePrice_DoubleBreak::MakeGreatBreak()
{
	HighAndLow highAndLow = CKLineAndMaTool::MakeHighAndLow(GetCodeId(), GetTimeType(), Now(), m_klinecount_open, false);

	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), GetTimeType(), Now());
	if (!thiskline) return false;

	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy && thiskline->high > highAndLow.high)
	{
		return true;
	}
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Sell && thiskline->low < highAndLow.low)
	{
		return true;
	}
	return false;
}

