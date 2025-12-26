#include "pch.h"
#include "BaseLine_Manual.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Factory_TShareEnv.h>
#include "KLineAndMaTool.h"
#include <Global.h>
CBaseLine_Manual::CBaseLine_Manual(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint)
	:CBaseLine(pStrategyParam, pContract, openPoint)
{
}

bool CBaseLine_Manual::FreshPricePair(time_t now)
{
	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());

	// 得到从启动程序到当下的K线
	IBKLinePtrs klinesTradeIn = GetKLinesFromSetup();
	HighAndLow highAndLowTradeIn = CHighFrequencyGlobalFunc::MakeHighAndLow(klinesTradeIn, false);

	if (!CanActivatePricePair(highAndLowTradeIn)) return false;

	if (MakeAndGet_DbLastOrder()->GetCoverCount(m_strategyIdHashId) == 0)
	{
		// 当日尚未交易过
		SetPricePair(highAndLowTradeIn);
	}
	else
	{
		// 当日曾经交易过, 得到上次交易的位置状态
		LinePositon_Type lastOrderLineType = GetLastOrderLinePositionType();
		if (GetLastOrderLinePositionType() == GetLinePositionType(highAndLowTradeIn))
		{
			// 上次委托到现在状态没有改变，不用改变pricePair，什么也不用做
			;
		}
		else
		{
			SetPricePair(highAndLowTradeIn);
		}
	}
	// 本策略不需要保存pricePair到磁盘
	// MakeAndGet_DbPricePairs()->SetStatus(FileStatus::Updated);
	return true;
}

bool CBaseLine_Manual::NeedFreshPricePair()
{
	// 本模式下，需要一直调整基准
	return true;
}

bool CBaseLine_Manual::CanActivatePricePair(const HighAndLow& highAndLowTradeIn)
{
	Tick_T lowPosM1 = CGetRecordNo::GetRecordNo(Time_Type::M1, highAndLowTradeIn.lowPos);
	Tick_T highPosM1 = CGetRecordNo::GetRecordNo(Time_Type::M1, highAndLowTradeIn.highPos);
	Tick_T nowM1 = CGetRecordNo::GetRecordNo(Time_Type::M1, Get_CurrentTime()->GetCurrentTime_millisecond());

	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		if (GetLinePositionType(highAndLowTradeIn.low) != LinePositon_Type::Out && nowM1 != lowPosM1) return true;
	}
	else
	{
		if (GetLinePositionType(highAndLowTradeIn.high) != LinePositon_Type::Out && nowM1 != highPosM1) return true;
	}
	return false;
}

IBKLinePtrs CBaseLine_Manual::GetKLinesFromSetup()
{
	TimePair timePair;
	timePair.beginPos = GetSetupTime() * 1000;
	timePair.endPos = CGetRecordNo::GetRecordNoEnd(Time_Type::M1, Get_CurrentTime()->GetCurrentTime_millisecond());

	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_MoreDay(GetCodeId(), Time_Type::M1, timePair);
	return klines;

}

double CBaseLine_Manual::TransParam(double param)
{
	return param;
	//if (param > 1)
	//{
	//	// 绝对值形式
	//	return param;
	//}

	//// 百分比形式
	//MakeLastDayKLine();
	//int priceStep = CGlobal::DToI(m_lastDay->close * param / m_pContract->minMove);

	//return priceStep * m_pContract->minMove;
}

void CBaseLine_Manual::MakeLastDayKLine()
{
	if (m_lastDay) return;

	Tick_T now = Get_CurrentTime()->GetCurrentTime_millisecond();

	size_t count = 1;
	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_MoreDay(m_pStrategyParam->key.targetCodeId, Time_Type::D1, now, count);
	if (klines.size() != count) return ;

	m_lastDay = klines.back();
	return;

}

void CBaseLine_Manual::SetPricePair(double openPrice, double stopPrice)
{
	m_pricePair->day = CHighFrequencyGlobalFunc::GetDayMillisec();
	m_pricePair->openPrice = openPrice;
	m_pricePair->stopLossPrice = stopPrice;
	return;
}

void CBaseLine_Manual::SetPricePair(const HighAndLow& highAndLowTradeIn)
{
	double param2 = TransParam(m_pStrategyParam->param2);
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		SetPricePair(highAndLowTradeIn.low + param2, highAndLowTradeIn.low);
	}
	else
	{
		SetPricePair(highAndLowTradeIn.high - param2, highAndLowTradeIn.high);
	}
	return;

}

LinePositon_Type CBaseLine_Manual::GetLinePositionType(double price)
{
	int priceStep = CGlobal::DToI(price / m_pContract->minMove);
	int line1Step = CGlobal::DToI(m_pStrategyParam->line1 / m_pContract->minMove);
	int line2Step = CGlobal::DToI(m_pStrategyParam->line2 / m_pContract->minMove);

	// 仅在开仓时才需要检查和调整基准
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		if (priceStep > line1Step) return LinePositon_Type::Out;
		if (priceStep <= line1Step && priceStep > line2Step) return LinePositon_Type::Middle;
		return LinePositon_Type::Deep;
	}
	else
	{
		if (priceStep < line1Step) return LinePositon_Type::Out;
		if (priceStep >= line1Step && priceStep < line2Step) return LinePositon_Type::Middle;
		return LinePositon_Type::Deep;

	}
}

LinePositon_Type CBaseLine_Manual::GetLastOrderLinePositionType()
{
	LocalOrderNo localOrderNo = MakeAndGet_DbLastOrder()->GetLastOrderNo(OrderKey(m_strategyIdHashId, StSubModule::SubCover));
	OneOrderPtr lastCoverOrder = MakeAndGet_DbInactiveOrder()->GetOne(localOrderNo);
	return GetLinePositionType(lastCoverOrder->orderPriceStrategy);
}

LinePositon_Type CBaseLine_Manual::GetLinePositionType(const HighAndLow& highAndLowTradeIn)
{
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		return GetLinePositionType(highAndLowTradeIn.low);
		
	}
	else
	{
		return GetLinePositionType(highAndLowTradeIn.high);
	}
}
