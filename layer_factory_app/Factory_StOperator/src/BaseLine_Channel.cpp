#include "pch.h"
#include "BaseLine_Channel.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <Factory_Log.h>
#include <Factory_QDatabase.h>
#include <Factory_TShareEnv.h>
#include "Factory_StOperator.h"
#include "CheckMa_Channel_Forward.h"
#include "KLineAndMaTool.h"
#include "Factory_BaseLine.h"
CBaseLine_Channel::CBaseLine_Channel(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint)
	:CBaseLine(pStrategyParam, pContract, openPoint)
{

}

// 按动态行情重新刷新m_pricePair的值
bool CBaseLine_Channel::FreshPricePair(time_t now)
{
	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());

	if (now == CGlobal::GetTimeByStr("20240717 09:58:00") * 1000)
	{
		int a = 0;
	}

	// 从文件读取高低点
	HighAndLow highAndLowTradeSide = GetHighAndLow_TradeSide();

	IBKLinePtrs klinesTradeIn = GetKLines_TradeIn();
	HighAndLow highAndLowTradeIn = CHighFrequencyGlobalFunc::MakeHighAndLow(klinesTradeIn, false);

	if (!CanFreshPricePair(highAndLowTradeSide, highAndLowTradeIn))
	{
		return false;
	}

	// 要求highAndLowTradeIn的高低点必须处于背离状态
	if (!IsMacdDiv(highAndLowTradeIn))
	{
		return false;
	}


	m_pricePair->day = CHighFrequencyGlobalFunc::GetDayMillisec();

	double param2 = TransParam(m_pStrategyParam->param2);

	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		m_pricePair->openPrice = highAndLowTradeIn.low + param2;
		m_pricePair->stopLossPrice = highAndLowTradeIn.low - 0;
	}
	else
	{
		m_pricePair->openPrice = highAndLowTradeIn.high - param2;
		m_pricePair->stopLossPrice = highAndLowTradeIn.high + 0;
	}
	MakeAndGet_DbPricePairs()->SetStatus(FileStatus::Updated);
	return true;
}

bool CBaseLine_Channel::NeedFreshPricePair()
{
	time_t currentDayTime = CHighFrequencyGlobalFunc::GetDayMillisec();

	// 日期不一致，需要调整pricePair
	if (m_pricePair->day != currentDayTime) return true;

	return true;

	if (Get_CurrentTime()->GetCurrentTime_millisecond() == CGlobal::GetTimeByStr("20240102 11:30:00") * 1000)
	{
		int a = 0;
	}
	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());



	// 因为8:29策略都会初始化各自的lastOrder和pricePair
	// 因此交易次数为零即表示当日没有过交易 需要调整pricePair
	if (MakeAndGet_DbLastOrder()->GetCoverCount(m_strategyIdHashId) == 0) return true;

	if (MakeAndGet_DbLastOrder()->GetCoverCount(m_strategyIdHashId) <= m_pStrategyParam->canChgBaseCount)
	{
		// 平仓次数不为零，判断当日虽然已经发送过开仓单是否仍需要调整pricePair
		double param2 = TransParam(m_pStrategyParam->line1);
		IBKLinePtrs klinesTradeIn = GetKLines_TradeIn();
		HighAndLow highAndLowTradeIn = CHighFrequencyGlobalFunc::MakeHighAndLow(klinesTradeIn, false);
		OneOrderPtr pLastOrder = GetDbLastOrder(StSubModule::SubCover);
		if (!pLastOrder)
		{
			printf("pLastOrder is null \n");
			Log_Print(LogLevel::Err, "pLastOrder is null");
			exit(-1);
		}

		// 平仓价格就是止损价格
		double lastStopLoss = pLastOrder->orderPriceStrategy;

		if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
		{
			if (highAndLowTradeIn.low > lastStopLoss - param2)
			{
				return false;
			}
		}
		else
		{
			if (highAndLowTradeIn.high < lastStopLoss + param2)
			{
				return false;
			}
		}

		return true;
	}


	return false;
}



IBKLinePtrs CBaseLine_Channel::GetKLines_TradeSide()
{
	time_t todayDayTimeMs = CHighFrequencyGlobalFunc::GetDayMillisec();

	TimePair timePair;
	if (m_pContract->securityType == SecurityType::FUT)
	{
		timePair.beginPos = todayDayTimeMs;
	}
	else
	{
		timePair.beginPos = todayDayTimeMs + CHighFrequencyGlobalFunc::MakeMilliSecondPart("04:00:00", 0);
	}
	timePair.endPos = todayDayTimeMs + m_openPoint;

	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair = timePair;


	IBKLinePtrs ret = CKLineAndMaTool::GetKLines_MoreDay(GetCodeId(), GetTimeType(), timePair);
	return ret;

}

IBKLinePtrs CBaseLine_Channel::GetKLines_TradeIn()
{
	TimePair timePair;
	timePair.beginPos = CHighFrequencyGlobalFunc::GetDayMillisec() + m_openPoint;
	timePair.endPos = CGetRecordNo::GetRecordNoEnd(Time_Type::M1, Get_CurrentTime()->GetCurrentTime_millisecond());

	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_MoreDay(GetCodeId(), Time_Type::M1, timePair);
	return klines;
}




HighAndLow CBaseLine_Channel::GetHighAndLow_TradeSide()
{
	if (MakeAndGet_HighLowPointOutside()->CanReadHighLowPointFromFile())
	{
		HighLowPoint highLowPoint;
		if (MakeAndGet_HighLowPointOutside()->GetHighLowPoint(CHighFrequencyGlobalFunc::GetDayMillisec(), highLowPoint))
		{
			HighAndLow ret;
			// ret.high = highLowPoint.highPoint - 10;
			ret.high = highLowPoint.highPoint;
			ret.highPos = CHighFrequencyGlobalFunc::GetDayMillisec() + m_openPoint;
			ret.highPosStr = CGlobal::GetTickTimeStr(ret.highPos);
			// ret.low = highLowPoint.lowPoint + 20;
			ret.low = highLowPoint.lowPoint;
			ret.lowPos = CHighFrequencyGlobalFunc::GetDayMillisec() + m_openPoint;
			ret.lowPosStr = CGlobal::GetTickTimeStr(ret.lowPos);
			return ret;
		}


		printf("can not find daytime \n");
		exit(-1);
	}
	else
	{
		if (m_pStrategyParam->useInterval)
		{
			IBKLinePtrs klinesTradeSide = GetKLines_TradeSide();
			return CHighFrequencyGlobalFunc::MakeHighAndLow(klinesTradeSide, false);

		}
		else
		{
			time_t openTime = CHighFrequencyGlobalFunc::GetDayMillisec() + m_openPoint;
			IBKLinePtr kline = CKLineAndMaTool::GetOneKLine(GetCodeId(), Time_Type::M1, openTime);
			HighAndLow ret;
			ret.high = kline->open;
			ret.highPos = kline->time;
			ret.highPosStr = CGlobal::GetTickTimeStr(ret.highPos);
			ret.low = kline->open;
			ret.lowPos = kline->time;
			ret.lowPosStr = CGlobal::GetTickTimeStr(ret.lowPos);
			return ret;

		}

	}



}

time_t CBaseLine_Channel::GetLastDayTimeMs()
{
	time_t todayDayTimeMs = CHighFrequencyGlobalFunc::GetDayMillisec();
	size_t count = 1;
	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_MoreDay(GetCodeId(), Time_Type::D1, todayDayTimeMs, count);
	if (klines.size() != 1) return 0;

	return klines.back()->time;
}

CheckMaPtr CBaseLine_Channel::MakeAndGet_CheckMa()
{
	if (!m_pCheckMa)
	{
		m_pCheckMa = std::make_shared<CCheckMa_Channel_Forward>(m_pStrategyParam);
	}
	return m_pCheckMa;
}

bool CBaseLine_Channel::CanFreshPricePair(const HighAndLow& highAndLowTradeSide, const HighAndLow& highAndLowTradeIn)
{
	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());

	if (!MakeAndGet_CheckMa()->Check(Get_CurrentTime()->GetCurrentTime_millisecond())) return false;

	m_pricePair->freshPart.freshTime = Get_CurrentTime()->GetCurrentTime_millisecond();

	double param1 = 0.0;
	if (!MakeAndGet_HighLowPointOutside()->CanReadHighLowPointFromFile())
	{
		if (MakeAndGet_CheckMa()->GetTrendType() == TrendType::AntiTrend)
		{
			param1 = TransParam(m_pStrategyParam->antiTrendParam1);
		}
		else
		{
			param1 = TransParam(m_pStrategyParam->proTrendParam1);
		}
	}
	m_pricePair->freshPart.trendType = MakeAndGet_CheckMa()->GetTrendType();
	m_pricePair->freshPart.param1 = param1;


	time_t lowPosM1 = CGetRecordNo::GetRecordNo(Time_Type::M1, highAndLowTradeIn.lowPos);
	time_t highPosM1 = CGetRecordNo::GetRecordNo(Time_Type::M1, highAndLowTradeIn.highPos);


	time_t nowM1 = CGetRecordNo::GetRecordNo(Time_Type::M1, Get_CurrentTime()->GetCurrentTime_millisecond());
	double minMove = m_pContract->minMove;

	time_t todayDayTimeMs = CHighFrequencyGlobalFunc::GetDayMillisec();
	time_t beginPos = todayDayTimeMs + CHighFrequencyGlobalFunc::MakeMilliSecondPart("08:30:00", 0);
	time_t endPos = todayDayTimeMs + CHighFrequencyGlobalFunc::MakeMilliSecondPart("09:30:00", 0);

	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		if (m_pStrategyParam->useTimeInterval && MakeAndGet_DbLastOrder()->GetCoverCount(m_strategyIdHashId) == 0)
		{
			// 第一次极值必须在指定时间区间
			if (highAndLowTradeIn.lowPos < beginPos || highAndLowTradeIn.lowPos > endPos) return false;
		}

		m_pricePair->freshPart.sideExtreamePrice = highAndLowTradeSide.low;
		m_pricePair->freshPart.sideExtreamePriceTime = highAndLowTradeSide.lowPos;
		m_pricePair->freshPart.targetPrice = highAndLowTradeSide.low - param1;
		if (CGlobal::DToI(highAndLowTradeIn.low / minMove) <= CGlobal::DToI((highAndLowTradeSide.low - param1) / minMove) && nowM1 != lowPosM1)
		{
			return true;
		}

	}
	else
	{
		if (m_pStrategyParam->useTimeInterval && MakeAndGet_DbLastOrder()->GetCoverCount(m_strategyIdHashId) == 0)
		{
			if (highAndLowTradeIn.highPos < beginPos || highAndLowTradeIn.highPos > endPos) return false;
		}
		m_pricePair->freshPart.sideExtreamePrice = highAndLowTradeSide.high;
		m_pricePair->freshPart.sideExtreamePriceTime = highAndLowTradeSide.highPos;
		m_pricePair->freshPart.targetPrice = highAndLowTradeSide.high + param1;
		if (CGlobal::DToI(highAndLowTradeIn.high / minMove) >= CGlobal::DToI((highAndLowTradeSide.high + param1) / minMove) && nowM1 != highPosM1)
		{
			return true;
		}
	}
	return false;
}




double CBaseLine_Channel::TransParam(double param)
{
	//if (m_pContract->securityType == SecurityType::STK)
	//{
	//	return param;
	//}

	if (param > 1)
	{
		// 绝对值形式
		return param;
	}

	// 百分比形式
	if (!MakeAndGet_CheckMa()->Check(Get_CurrentTime()->GetCurrentTime_millisecond()))
	{
		Log_Print(LogLevel::Err, "CheckMa err");
		exit(-1);
	}
	//int priceStep = CGlobal::DToI(MakeAndGet_CheckMa()->GetAtrValue() * param / m_pContract->minMove);
	int priceStep = CGlobal::DToI(MakeAndGet_CheckMa()->GetLastDayClose() * param / m_pContract->minMove);

	return priceStep * m_pContract->minMove;
}

OneOrderPtr CBaseLine_Channel::GetDbLastOrder(StSubModule module)
{
	OrderKey orderkey(m_strategyIdHashId, module);
	LocalOrderNo localOrderNo = MakeAndGet_DbLastOrder()->GetLastOrderNo(orderkey);
	return MakeAndGet_DbInactiveOrder()->GetOne(localOrderNo);

}

bool CBaseLine_Channel::IsMacdDiv(const HighAndLow& highAndLowTradeIn)
{
	if (Get_CurrentTime()->GetCurrentTime_millisecond() == CGlobal::GetTimeByStr("20240102 10:00:00") * 1000)
	{
		int a = 0;
	}
	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());

	int truecount = 0;
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		// 准备买入开仓，低点必须处于背离位置
		
		DivergenceType type = DivergenceType::Normal;
		type = GetDivType(Time_Type::H1, highAndLowTradeIn);
		if (type == DivergenceType::Bottom || type == DivergenceType::BottomSub)
		{
			truecount++;
			//return true;
		}
		type = GetDivType(Time_Type::M30, highAndLowTradeIn);
		if (type == DivergenceType::Bottom || type == DivergenceType::BottomSub)
		{
			truecount++;
			//return true;
		}
		type = GetDivType(Time_Type::M15, highAndLowTradeIn);
		if (type == DivergenceType::Bottom || type == DivergenceType::BottomSub)
		{
			truecount++;
			//return true;
		}
		type = GetDivType(Time_Type::M1, highAndLowTradeIn);
		if (type == DivergenceType::Bottom || type == DivergenceType::BottomSub)
		{
			truecount++;
			//return true;
		}
		if (truecount >= 2) return true;
	}
	else
	{
		// 准备卖出开仓，高点必须处于背离位置
		DivergenceType type = DivergenceType::Normal;
		type = GetDivType(Time_Type::H1, highAndLowTradeIn);
		if (type == DivergenceType::Top || type == DivergenceType::TopSub)
		{
			truecount++;
			//return true;
		}

		type = GetDivType(Time_Type::M30, highAndLowTradeIn);
		if (type == DivergenceType::Top || type == DivergenceType::TopSub)
		{
			truecount++;
			//return true;
		}

		type = GetDivType(Time_Type::M15, highAndLowTradeIn);
		if (type == DivergenceType::Top || type == DivergenceType::TopSub)
		{
			truecount++;
			//return true;
		}

		type = GetDivType(Time_Type::M1, highAndLowTradeIn);
		if (type == DivergenceType::Top || type == DivergenceType::TopSub)
		{
			truecount++;
			//return true;
		}
		if (truecount >= 2) return true;
	}

	return false;
}

DivergenceType CBaseLine_Channel::GetDivType(Time_Type timeType, const HighAndLow& highAndLowTradeIn)
{
	Tick_T time;
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		// 准备买入开仓，低点必须处于背离位置
		time = highAndLowTradeIn.lowPos;
	}
	else
	{
		// 准备卖出开仓，高点必须处于背离位置
		time = highAndLowTradeIn.highPos;
	}
	//if (CGetRecordNo::TimesInSameBar(timeType, time, Get_CurrentTime()->GetCurrentTime_millisecond()))
	//{
	//	// 极值点与当前时间处于同一个bar，不可使用
	//	return DivergenceType::Normal;
	//}
	return MakeAndGet_Container_MacdDiv()->GetMacdDivAtTime(timeType, time);
}

bool CBaseLine_Channel::IsTrain(Time_Type timeType, const HighAndLow& highAndLowTradeIn)
{
	Tick_T time;
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		// 准备买入开仓，低点必须处于背离位置
		time = highAndLowTradeIn.lowPos;
	}
	else
	{
		// 准备卖出开仓，高点必须处于背离位置
		time = highAndLowTradeIn.highPos;
	}

	MacdValue macdValue = MakeAndGet_Container_Macd()->GetMacdAtTime(timeType, time);

	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy && macdValue.dif > macdValue.dea)
	{
		return true;
	}
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Sell && macdValue.dif < macdValue.dea)
	{
		return true;
	}
	return false;


}
