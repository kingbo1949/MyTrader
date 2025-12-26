#include "pch.h"
#include "MakePrice_DayBreakManual.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Factory_TShareEnv.h>
#include <Global.h>
#include <Factory_Log.h>
#include "KLineAndMaTool.h"
#include "Factory_BaseLine.h"
CMakePrice_DayBreakManual::CMakePrice_DayBreakManual(const SubModuleParams& stParams)
	:CMakePrice_Real(stParams)
{
	m_coverPoint = CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:00:00", 0);

	if (m_stParams.openOrCover == RoundAction::Open_Round)
	{
		Make_BaseLine(m_pStrategyParam, m_stParams.contract, 0);
	}

	m_pBaseLine = Get_BaseLine(m_stParams.strategyIdHashId);
}

bool CMakePrice_DayBreakManual::GetOpenPrice(double& price, RealPriceType& priceType)
{
	// 计算开仓价格
	if (Get_Sys_Status() != Thread_Status::Running) return false;

	Tick_T todayDayTimeMs = CHighFrequencyGlobalFunc::GetDayMillisec();

	// 手动型策略需要手动初始化lastOrder和pricePair

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

	double openPrice = pricePair->openPrice;
	if (m_stParams.buyOrSell == BuyOrSell::Buy)
	{
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

bool CMakePrice_DayBreakManual::GetCoverPrice(double& price, RealPriceType& priceType)
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

	// 按价格检查是否需要平仓
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
	//time_t coverBaseTime = CHighFrequencyGlobalFunc::GetDayMillisec() + m_coverPoint;
	//if (Now() >= coverBaseTime)
	//{
	//	price = thiskline->open;
	//	priceType = RealPriceType::Eat;
	//	return true;
	//}


	return false;
}
