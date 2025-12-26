#include "pch.h"
#include "MakePrice_BreakInDay.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Factory_QDatabase.h>
#include <Factory_Log.h>
#include <Global.h>
#include "KLineAndMaTool.h"
CMakePrice_BreakInDay::CMakePrice_BreakInDay(const SubModuleParams& stParams)
	:CMakePrice_Real(stParams)
{

}

bool CMakePrice_BreakInDay::GetOpenPrice(double& price, RealPriceType& priceType)
{
	if (Get_Sys_Status() != Thread_Status::Running) return false;

	priceType = RealPriceType::MidPoint;
	return GetOpenCoverPrice(price);

}

bool CMakePrice_BreakInDay::GetCoverPrice(double& price, RealPriceType& priceType)
{
	priceType = RealPriceType::MidPoint;
	if (MakeAndGet_TradePointEnv()->NearCloseForForceCover(m_stParams.targetCodeId) || Get_Sys_Status() == Thread_Status::QuickCover)
	{
		// 处于札平时间段，或者强平状态，总是挂在0号位置
		std::string codeId = m_pStrategyParam->key.targetCodeId;
		Time_Type timetype = m_pStrategyParam->timetype;
		time_t currentTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();
		IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timetype, currentTickTime);
		if (!thisKline) return false;

		price = thisKline->open;
		return true;
	}
	else
	{
		return GetOpenCoverPrice(price);


	}

}

bool CMakePrice_BreakInDay::GetOpenCoverPrice(double& price)
{
	std::string codeId = m_pStrategyParam->key.targetCodeId;
	Time_Type timetype = m_pStrategyParam->timetype;
	time_t currentTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();
	IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timetype, currentTickTime);
	if (!thisKline) return false;


	// 使用K线的数目
	size_t count = 20;
	if (m_stParams.openOrCover == RoundAction::Cover_Round)
	{
		count = 10;
	}
	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_OneDay(codeId, timetype, currentTickTime, count);
	if (klines.size() != count) return false;

	HighAndLow highAndLow;
	highAndLow = CKLineAndMaTool::GetExtremePrice(klines);

	if (m_stParams.buyOrSell == BuyOrSell::Buy)
	{
		if (thisKline->high < highAndLow.high)
		{
			return false;
		}
		else
		{
			price = highAndLow.high;;
		}

	}
	else
	{
		if (thisKline->low > highAndLow.low)
		{
			return false;
		}
		else
		{
			price = highAndLow.low;
		}
		

	}

	//if (back != 0)
	//{
	//	Log_Print(LogLevel::Info, fmt::format("currentTime = {}, thisKLine = {}",
	//		CGlobal::GetTickTimeStr(currentTickTime).c_str(),
	//		CKLineTool::GetKLineStr(thisKline).c_str()
	//	));

	//	Log_Print(LogLevel::Info, fmt::format("before thisline, {}->{}, [{}, {}], send price = {}",
	//		CGlobal::GetTickTimeStr(klines[0]->time).c_str(),
	//		CGlobal::GetTickTimeStr(klines.back()->time).c_str(),
	//		highAndLow.low,
	//		highAndLow.high,
	//		back
	//	));
	//}

	return true;


}

