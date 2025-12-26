#include "pch.h"
#include "Setup_TRunTime.h"
#include <Factory_Strategys.h>
#include <Factory_IBJSon.h>
#include <Factory_Topics.h>
#include <Factory_Log.h>
#include <Factory_IBJSon.h>
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include <Factory_QShareEnv.h>
#include <Factory_HashEnv.h>
#include <Factory_UnifyInterface.h>
#include "Factory_Setup.h"
CSetup_TRunTime::CSetup_TRunTime()
	:CSetup_Real(CurrentTimeType::For_Real)
{

}

void CSetup_TRunTime::Init_Factory_Strategys()
{
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	Make_RunTimeStrategys(strategyParams);

	return;
}


void CSetup_TRunTime::RegisterTCallbackObject()
{
	MakeAndGet_Topic_OnTrade()->SubscribeTick_OnTrade(On_TradeChg_Strategy);
}

void CSetup_TRunTime::Init_DaemonObject()
{
	Make_DaemonByTick(SetupType::RealTrader);
	return;


}

void CSetup_TRunTime::Init_QCallbackObject()
{
	Make_QCallbackObject(SetupType::RealTrader);
	return;
}

void CSetup_TRunTime::Init_Factory_TShareEnv()
{
	MakeAndGet_DbPositionSize();
	MakeAndGet_DbDeal();
	MakeAndGet_DbInactiveOrder();
	MakeAndGet_DbActiveOrder();
	MakeAndGet_DbActiveAction();

	// 开始装载持仓
	PositionSizePtrs positionsizes;
	MakeAndGet_JSonPositionSize()->Load_PositionSizes(positionsizes);

	for (auto positionsize : positionsizes)
	{
		MakeAndGet_DbPositionSize()->AddOne(positionsize);
	}

	// 开始装载当日成交
	OneDealPtrs deals;
	MakeAndGet_JSonDeal()->Load_Deals(deals);
	for (auto deal : deals)
	{
		if (CHighFrequencyGlobalFunc::GetDayMillisec(deal->dealTickTime) != CHighFrequencyGlobalFunc::GetDayMillisec()) continue;

		MakeAndGet_DbDeal()->AddOne(deal);
	}

	// 开始装载LastOrder环境
	LoadLastOrderFromFile();

	// 装载pricePair环境
	LoadPricePairFromFile();


	return;

}

void CSetup_TRunTime::Init_Factory_QShareEnv()
{
	Make_CodeIdManager(SetupType::RealTrader);
	MakeAndGet_QEnvManager();
}

void CSetup_TRunTime::Init_Factory_StrategyHashEnv()
{
	// 初始化策略哈希属性表
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	Make_StrategyParamEnv(strategyParams);

}

void CSetup_TRunTime::Init_Factory_UnifyInterface()
{
	Make_IBApi(SetupType::RealTrader);
	return;
}

void CSetup_TRunTime::RunIBApi()
{
	Get_IBApi()->Init();
	Get_IBApi()->Connect();

	// 订阅行情
	IbContractPtrs contracts = GetStrategyCodeId(CodeIdType::ForSubscribeQ);
	Get_IBApi()->SubscribeQuote(contracts);
	return;

}

void CSetup_TRunTime::LoadLastOrderFromFile()
{
	std::map<OrderKey, Tick_T> lastOrderDealTimes;
	std::map<OrderKey, LocalOrderNo> lastOrderNos;
	std::map<StrategyIdHashId, size_t> coverCounts;
	MakeAndGet_JSonLastOrder()->Load(lastOrderDealTimes, lastOrderNos, coverCounts);
	for (const auto& item : lastOrderDealTimes)
	{
		MakeAndGet_DbLastOrder()->UpdateLastOrderDealTime(item.first, item.second);
	}
	for (const auto& item : lastOrderNos)
	{
		MakeAndGet_DbLastOrder()->UpdateLastOrderNo(item.first, item.second);
	}
	for (const auto& item : coverCounts)
	{
		MakeAndGet_DbLastOrder()->UpdateCoverCount(item.first, item.second);
	}
	return;
}

void CSetup_TRunTime::LoadPricePairFromFile()
{
	std::map<StrategyIdHashId, PricePairPtr> pricePairs;
	MakeAndGet_JSonPricePair()->Load_PricePairs(pricePairs);
	for (const auto& item : pricePairs)
	{
		PricePairPtr pPricePair = MakeAndGet_DbPricePairs()->GetPricePair(item.first);
		pPricePair->day = item.second->day;
		pPricePair->openPrice = item.second->openPrice;
		pPricePair->stopLossPrice = item.second->stopLossPrice;
	}


}



