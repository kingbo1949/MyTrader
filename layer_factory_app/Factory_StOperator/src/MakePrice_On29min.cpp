#include "pch.h"
#include "MakePrice_On29min.h"
#include <Factory_UnifyInterface.h>
#include <GetRecordNo.h>
#include <Factory_TShareEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <Log.h>
#include <Factory_Log.h>
#include "KLineAndMaTool.h"
CMakePrice_On29min::CMakePrice_On29min(const SubModuleParams& stParams)
	:CMakePrice_Real(stParams)
{
	m_klinecount = 5;
}

bool CMakePrice_On29min::GetOpenPrice(double& price, RealPriceType& priceType)
{
	if (Get_Sys_Status() != Thread_Status::Running) return false;

	if (!CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now())) return false;
	if (!CKLineAndMaTool::GetOneKLine(GetCodeId(), GetTimeType(), Now())) return false;

	if (!IsRegularTradeTime()) return false;


	// 判断二次开仓
	if (IsTwiceOpen()) return false;

	// 检查是否29分或者59分
	if (!IsCanOpenTime()) return false;

	m_highAndLow = CKLineAndMaTool::MakeHighAndLow(GetCodeId(), GetTimeType(), Now(), m_klinecount, false);

	priceType = RealPriceType::MidPoint;


	// 检查1分钟线上是否仍然是突破状态
	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (m_stParams.buyOrSell == BuyOrSell::Buy && thiskline->open > m_highAndLow.high)
	{
		price = std::max(thiskline->open, m_highAndLow.high) ;
		return true;
	}
	if (m_stParams.buyOrSell == BuyOrSell::Sell && thiskline->open < m_highAndLow.low)
	{
		price = std::min(thiskline->open, m_highAndLow.low);
		return true;
	}
	return false;
}

bool CMakePrice_On29min::GetCoverPrice(double& price, RealPriceType& priceType)
{
	// 平仓在开盘价格，所以先判断平仓时间后判断止损
	// 判断是否到达自然平仓时间

	if (!CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now())) return false;
	if (!CKLineAndMaTool::GetOneKLine(GetCodeId(), GetTimeType(), Now())) return false;


	priceType = RealPriceType::MidPoint;
	if (IsCanCoverTime())
	{
		// 已到平仓时间
		IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
		price = thiskline->open;
		return true;
	}
	// 判断是否需要止损
	if (NeedStopCover(price))
	{
		return true;
	}

	return false;
}

bool CMakePrice_On29min::IsCanOpenTime()
{
	time_t kline_time_30m = CGetRecordNo::GetRecordNo(Time_Type::M30, Now() );
	time_t kline_time_1m = CGetRecordNo::GetRecordNo(Time_Type::M1, Now());

	if (kline_time_30m + 28 * 60 * 1000 < kline_time_1m)
	{
		return true;
	}
	return false;
}

bool CMakePrice_On29min::IsCanCoverTime()
{
	time_t kline_time_30m = CGetRecordNo::GetRecordNo(Time_Type::M30, Now());
	time_t kline_time_1m = CGetRecordNo::GetRecordNo(Time_Type::M1, Now());

	if (kline_time_30m + 1 * 60 * 1000 == kline_time_1m)
	{
		return true;
	}
	return false;
}

bool CMakePrice_On29min::IsTwiceOpen()
{
	time_t kline_time_1m = CGetRecordNo::GetRecordNo(Time_Type::M1, Now());
	if (Now() - kline_time_1m > 15 * 1000) return true;

	return false;
}

double CMakePrice_On29min::GetPsPrice()
{
	PositionSizePtr ps = MakeAndGet_DbPositionSize()->GetOne(m_stParams.strategyIdHashId);
	if (!ps)
	{
		Log_Print(LogLevel::Err, fmt::format("want to cover, ps is null, {}", m_pStrategyParam->key.ToStr().c_str()));
		exit(-1);
	}
	IBKLinePtr openkline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, ps->openTickTime);
	return openkline->open;


}

bool CMakePrice_On29min::NeedStopCover(double& price)
{
	// 得到持仓价格 
	double psPrice = GetPsPrice();

	// 止损价位
	int stopstep = 10;
	IBKLinePtr thiskline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, Now());
	if (m_stParams.buyOrSell == BuyOrSell::Buy)
	{
		// 买平空头
		if (thiskline->high - psPrice > stopstep * m_stParams.contract->minMove)
		{
			price = psPrice + stopstep * m_stParams.contract->minMove;

			//string str = fmt::format("stop loss! buy cover {} ",GetTickTimeStr(Now()).c_str());
			//int modSecond = 1;
			//Log_AsyncPrintRotatingFile("stoploss.log", str + "\n", modSecond, true);



			return true;
		}
	}
	if (m_stParams.buyOrSell == BuyOrSell::Sell)
	{
		// 卖平多头
		if (psPrice - thiskline->low > stopstep * m_stParams.contract->minMove)
		{
			price = psPrice - stopstep * m_stParams.contract->minMove;

			//string str = fmt::format("stop loss! sell cover {} ", GetTickTimeStr(Now()).c_str());
			//int modSecond = 1;
			//Log_AsyncPrintRotatingFile("stoploss.log", str + "\n", modSecond, true);

			return true;
		}
	}
	return false;
}
bool CMakePrice_On29min::IsRegularTradeTime()
{
	// 温哥华05:00:00 - 13:50:00
	// 美东08:00:00 - 17:50:00

	// 零点毫秒
	time_t daytime = CHighFrequencyGlobalFunc::GetDayMillisec();
	time_t beginPos = daytime + 8 * 60 * 60 * 1000;
	time_t endPos = daytime + 17 * 60 * 60 * 1000 + 50 * 60 * 1000;
	if (Now() < beginPos || Now() > endPos) return false;
	return true;

}
