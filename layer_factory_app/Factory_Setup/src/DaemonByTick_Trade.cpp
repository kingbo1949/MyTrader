#include "pch.h"
#include "DaemonByTick_Trade.h"
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_TShareEnv.h>
#include <Factory_IBJSon.h>
#include <Global.h>
CDaemonByTick_Trade::CDaemonByTick_Trade()
{
	m_lastUpdateSecond_tradePoint = 0;
	m_lastUpdateSecond_save = 0;

}

void CDaemonByTick_Trade::Execute()
{

	Handle_Log();
	Handle_TradePoint();
	Handle_SaveAll();


}


void CDaemonByTick_Trade::Handle_TradePoint()
{
	if (Get_CurrentTime()->GetCurrentTime_second() - m_lastUpdateSecond_tradePoint > 5)
	{
		MakeAndGet_TradePointEnv()->CheckAndSetStatus();

		m_lastUpdateSecond_tradePoint = Get_CurrentTime()->GetCurrentTime_second();
	}
	return;

}

void CDaemonByTick_Trade::Handle_SaveAll()
{
	

	if (Get_CurrentTime()->GetCurrentTime_second() - m_lastUpdateSecond_save > 1)
	{
		SavePositionSize();
		SaveDeal();

		if (MakeAndGet_JSonBroker()->LoadBroker()->setupType == SetupType::RealTrader)
		{
			SaveLastOrder();
			SavePricePairs();

		}

		m_lastUpdateSecond_save = Get_CurrentTime()->GetCurrentTime_second();
	}
	return;

}

void CDaemonByTick_Trade::SavePositionSize()
{
	if (MakeAndGet_DbPositionSize()->GetStatus() == FileStatus::Saved) return;

	PositionSizePtrs pss = MakeAndGet_DbPositionSize()->GetAll();
	MakeAndGet_JSonPositionSize()->Save_PositionSizes(pss);
	MakeAndGet_DbPositionSize()->SetStatus(FileStatus::Saved);

	return;
}


void CDaemonByTick_Trade::SaveDeal()
{
	if (MakeAndGet_DbDeal()->GetStatus() == FileStatus::Saved) return;

	OneDealPtrs deals = MakeAndGet_DbDeal()->GetAll();
	MakeAndGet_JSonDeal()->Save_Deals(deals);
	MakeAndGet_DbDeal()->SetStatus(FileStatus::Saved);
	return;
}

void CDaemonByTick_Trade::SaveLastOrder()
{
	if (MakeAndGet_DbLastOrder()->GetStatus() == FileStatus::Saved) return;

	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	std::map<OrderKey, Tick_T> lastOrderDealTimes;
	std::map<OrderKey, LocalOrderNo> lastOrderNos;
	std::map<StrategyIdHashId, size_t> coverCounts;
	MakeAndGet_DbLastOrder()->GetAll(lastOrderDealTimes, lastOrderNos, coverCounts);
	MakeAndGet_JSonLastOrder()->Save(lastOrderDealTimes, lastOrderNos, coverCounts);
	MakeAndGet_DbLastOrder()->SetStatus(FileStatus::Saved);
	return;
}

void CDaemonByTick_Trade::SavePricePairs()
{
	if (MakeAndGet_DbPricePairs()->GetStatus() == FileStatus::Saved) return;

	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	std::map<StrategyIdHashId, PricePairPtr> pricePairs;
	MakeAndGet_DbPricePairs()->GetAll(pricePairs);
	MakeAndGet_JSonPricePair()->Save_PricePairs(pricePairs);
	MakeAndGet_DbPricePairs()->SetStatus(FileStatus::Saved);
	return;

}

