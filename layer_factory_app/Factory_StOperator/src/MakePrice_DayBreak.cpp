#include "pch.h"
#include "MakePrice_DayBreak.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Factory_TShareEnv.h>
#include <Global.h>
#include <Factory_Log.h>
#include "KLineAndMaTool.h"
#include "Factory_BaseLine.h"
CMakePrice_DayBreak::CMakePrice_DayBreak(const SubModuleParams& stParams)
	:CMakePrice_Real(stParams)
{
	m_openPoint = CHighFrequencyGlobalFunc::MakeMilliSecondPart("08:30:00", 0);
	m_coverPoint = CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:00:00", 0);

	if (m_stParams.openOrCover == RoundAction::Open_Round)
	{
		Make_BaseLine(m_pStrategyParam, m_stParams.contract, m_openPoint);
	}

	m_pBaseLine = Get_BaseLine(m_stParams.strategyIdHashId);
}

bool CMakePrice_DayBreak::GetOpenPrice(double& price, RealPriceType& priceType)
{
	// 计算开仓价格
	if (Get_Sys_Status() != Thread_Status::Running) return false;

	Tick_T todayDayTimeMs = CHighFrequencyGlobalFunc::GetDayMillisec();

	// 按策略初始化lastOrder和pricePair
	time_t resetPos = todayDayTimeMs + CHighFrequencyGlobalFunc::MakeMilliSecondPart("08:29:00", 0);
	if (NowM1() == resetPos)
	{
		MakeAndGet_DbLastOrder()->Clear(m_stParams.strategyIdHashId);
		MakeAndGet_DbPricePairs()->Clear(m_stParams.strategyIdHashId);
	}

	// 是否达到最大交易次数
	if (MakeAndGet_DbLastOrder()->GetCoverCount(m_stParams.strategyIdHashId) >= m_pStrategyParam->maxTradeCount) return false;

	if (!IsCanOpenTime()) return false;

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
	// 计算开仓价格
	// 得到开平仓基准
	PricePairPtr pricePair = m_pBaseLine->MakeAndGetLine(Now(), m_stParams.openOrCover);
	if (!pricePair) return false;

	// 日期不一致，需要调整pricePair
	if (pricePair->day != todayDayTimeMs)
	{
		Log_Print(LogLevel::Err, fmt::format("{} pricePair->day != todayDayTimeMs", m_pStrategyParam->key.ToStr().c_str()));
		return false;
	}


	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (!thiskline) return false;

	//MacdValue macdValue = GetMacd_LastM30();
	//if (std::isnan(macdValue.dif))
	//{
	//	printf("macdValue.dif is NaN \n");
	//	return false;
	//	//exit(-1);
	//}


	double openPrice = pricePair->openPrice;
	if (m_stParams.buyOrSell == BuyOrSell::Buy)
	{
		//if (thiskline->high > openPrice && thiskline->low < openPrice && macdValue.dif < 10)
		if (thiskline->high > openPrice && thiskline->low < openPrice)
		{
			price = openPrice;
			priceType = RealPriceType::Eat;
			return true;
		}
		return false;

	}
	else
	{
		//if (thiskline->high > openPrice && thiskline->low < openPrice && macdValue.dif > 40)
		if (thiskline->high > openPrice && thiskline->low < openPrice)
		{
			price = openPrice;
			priceType = RealPriceType::Eat;
			return true;
		}
		return false;
	}
	return false;
}

bool CMakePrice_DayBreak::GetCoverPrice(double& price, RealPriceType& priceType)
{
	time_t lastOpenTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));
	lastOpenTime = CGetRecordNo::GetRecordNo(Time_Type::M1, lastOpenTime);

	if (NowM1() == lastOpenTime)
	{
		// 不可在开仓BAR平仓
		return false;
	}

	// 得到开平仓基准
	PricePairPtr pricePair = m_pBaseLine->MakeAndGetLine(Now(), m_stParams.openOrCover);
	if (!pricePair) return false;

	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (!thiskline) return false;

	std::string timestr1 = CGlobal::GetTickTimeStr(Now());

	// 按价格检查是否需要止损平仓
	double coverPrice = pricePair->stopLossPrice;
	if (m_stParams.buyOrSell == BuyOrSell::Buy)
	{
		if (thiskline->high > coverPrice)
		{
			price = coverPrice;
			priceType = RealPriceType::Eat;
			return true;
		}
	}
	else
	{
		if (thiskline->low < coverPrice)
		{
			price = coverPrice;
			priceType = RealPriceType::Eat;
			return true;
		}
	}


	// 按时间检查是否平仓
	//if (IsCanCoverTime(pricePair))
	//{
	//	price = thiskline->open;
	//	priceType = RealPriceType::Eat;
	//	return true;
	//}

	time_t coverBaseTime = CHighFrequencyGlobalFunc::GetDayMillisec() + m_coverPoint;
	if (Now() >= coverBaseTime)
	{
		price = thiskline->open;
		priceType = RealPriceType::Eat;
		return true;
	}


	return false;
}


bool CMakePrice_DayBreak::IsCanOpenTime()
{
	// 温哥华05:00:00 - 13:50:00
	// 美东08:00:00 - 17:50:00

	// 零点毫秒
	time_t daytime = CHighFrequencyGlobalFunc::GetDayMillisec();
	//time_t beginPos = daytime + 8 * 60 * 60 * 1000;
	//time_t endPos = daytime + 17 * 60 * 60 * 1000  + 50 * 60 * 1000;

	time_t beginPos = daytime + m_openPoint;
	//time_t beginPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("08:30:00", 0);
	time_t endPos = 0;
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		//endPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("13:30:00", 0);
		endPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("15:30:00", 0);
	}
	else
	{
		endPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("15:30:00", 0);
		//endPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("13:30:00", 0);
	}


	if (Now() < beginPos || Now() > endPos) return false;
	return true;
}

int CMakePrice_DayBreak::GetProfitStep(PricePairPtr pricePair)
{
	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (!thiskline) return 0;

	double profit = 0.0;
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		profit = thiskline->low - pricePair->openPrice;
	}
	else
	{
		profit = pricePair->openPrice - thiskline->high;
	}
	return CGlobal::DToI(profit / m_stParams.contract->minMove);
}

bool CMakePrice_DayBreak::IsCanCoverTime(PricePairPtr pricePair)
{
	time_t daytime = CHighFrequencyGlobalFunc::GetDayMillisec();
	time_t coverBaseTime_Begin = daytime + m_coverPoint;
	time_t coverBaseTime_End = coverBaseTime_Begin + 30 * 60 * 1000;
	if (Now() < coverBaseTime_Begin || Now() >= coverBaseTime_End) return false;

	int leastProfitPrice = 100;
	int leastProfitPriceStep = CGlobal::DToI(leastProfitPrice / m_stParams.contract->minMove);

	if (GetProfitStep(pricePair) < leastProfitPriceStep)
	{
		// 盈利不够，平仓
		return true;
	}

	if (daytime == pricePair->day)
	{
		// 盈利足够大，当日不平仓
		return false;
	}
	return true;
}

MacdValue CMakePrice_DayBreak::GetMacd_LastM30()
{
	//return MakeAndGet_MacdCalculators()->GetMACDAtTime(Time_Type::M30, LastM30());
	return MacdValue();

}
