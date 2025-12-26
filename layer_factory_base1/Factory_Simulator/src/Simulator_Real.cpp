#include "pch.h"
#include "Simulator_Real.h"
#include "Factory_Simulator.h"
#include <Factory_Log.h>
#include <Factory_TShareEnv.h>
#include <Factory_Callback.h>
#include <Factory_QShareEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Factory_IBJSon.h>
#include "SimulatorDefine.h"
#include <Global.h>
#include <Factory_HashEnv.h>


CSimulator_Real::CSimulator_Real()
{
	m_lineType = MakeAndGet_JSonTimeZone(TimeZone_Type::For_Simulator)->Load_TimeZone()->lineType;
	if (m_lineType == LineType::UseMa)
	{
		Log_Print(LogLevel::Err, "CSimulator_Real can not use ma linetype");
		exit(-1);
	}
}

void CSimulator_Real::MeetDeal()
{
	std::set<LocalOrderNo>	dealedOrders;

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto pos : m_orders)
		{
			double dealPrice = 0.0;
			bool success = MeetDeal(pos.second, dealPrice);
			if (!success) continue;

			std::string tem = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
			//string str = fmt::format("meet one deal, {} {}", tem.c_str(), pos. );

			// 成交撮合成功
			// 制作委托回报
			OnOrderMsg onOrderMsg;
			onOrderMsg.orderStatus = OrderStatus::AllDealed;
			onOrderMsg.todayDealedAvgPrice = dealPrice;
			onOrderMsg.todayDealedVol = pos.second->orderQuantity;
			onOrderMsg.pOriginalOrder = MakeAndGet_DbActiveOrder()->GetOne(pos.second->orderKey , pos.first);

			// 制作成交回报
			OnTradeMsg onTradeMsg;
			onTradeMsg.dealNo = int(MakeAndGet_SeriesNoMaker()->GetSeriesNo());
			onTradeMsg.dealPrice = dealPrice;
			onTradeMsg.dealQuantity = pos.second->orderQuantity;
			onTradeMsg.pOriginalOrder = MakeAndGet_DbActiveOrder()->GetOne(pos.second->orderKey, pos.first);

			dealedOrders.insert(pos.first);

			// 添加到OnRtnQueue
			PushOnOrderMsgs(onOrderMsg);
			PushOnTradeMsgs(onTradeMsg);

		}


		// 从map中删除已成交单
		for (auto dealed : dealedOrders)
		{
			auto pos = m_orders.find(dealed);
			m_orders.erase(pos);
		}

	}


}

bool CSimulator_Real::MeetDeal(OneOrderOfSimulatorPtr porder, double& dealPrice)
{
	if (m_lineType == LineType::UseKLine)
	{
		// 使用k线，直接撮合成交
		dealPrice = porder->orderPriceReal;
		return true;
	}
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(porder->codeHashId);
	int orderPriceStep = CGlobal::DToI(porder->orderPriceReal / contract->minMove);

	TickEnvPtr pTickEnv = MakeAndGet_QEnvManager()->GetEnvOneCode(porder->codeHashId)->GetTicEnv();
	int priceStatus = CHighFrequencyGlobalFunc::GetPriceStatus(pTickEnv->pTick, porder->buyOrSell, orderPriceStep);

	bool back = MakeAndGet_MeetDeal()->CanDeal(priceStatus);
	if (!back)
	{
		return false;
	}
	std::string timestr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond() );
	dealPrice = MakeAndGet_MeetDeal()->GetDealPrice(porder->buyOrSell, porder->orderPriceReal, priceStatus, pTickEnv->pTick, contract->minMove);

	return back;

}



void CSimulator_Real::RecOneOrder(OneOrderOfSimulatorPtr pOrder)
{
	OnOrderMsg onOrderMsg;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!pOrder)
		{
			Log_Print(LogLevel::Err, "porder is null");
			exit(1);
		}

		auto pos = m_orders.find(pOrder->localOrderNo);
		if (pos != m_orders.end())
		{
			Log_Print(LogLevel::Err, fmt::format("{}, order exist! ", pOrder->localOrderNo));
			exit(1);
		}
		m_orders[pOrder->localOrderNo] = pOrder;

		// 推送回报【已报】
		onOrderMsg.todayDealedVol = 0.0;

		onOrderMsg.pOriginalOrder = MakeAndGet_DbActiveOrder()->GetOne(pOrder->orderKey, pOrder->localOrderNo);
		if (!onOrderMsg.pOriginalOrder)
		{
			Log_Print(LogLevel::Err, "onOrderMsg.pOrder is null");
			exit(1);
		}
		onOrderMsg.orderStatus = OrderStatus::SomeDealed;

	}
	PushOnOrderMsgs(onOrderMsg);
	return;


}

void CSimulator_Real::RecOneCancel(int localOrderNo)
{
	OnOrderMsg onOrderMsg;
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto pos = m_orders.find(localOrderNo);
		if (pos == m_orders.end())
		{
			Log_Print(LogLevel::Err, fmt::format("{}, order not exist! ", localOrderNo));
			exit(1);
		}
		OneOrderOfSimulatorPtr porder = pos->second;
		m_orders.erase(pos);

		// 推送回报【已撤单】
		onOrderMsg.pOriginalOrder = MakeAndGet_DbActiveOrder()->GetOne(porder->orderKey, porder->localOrderNo);
		if (!onOrderMsg.pOriginalOrder)
		{
			Log_Print(LogLevel::Err, "onOrderMsg.pOrder is null");
			exit(1);
		}
		onOrderMsg.orderStatus = OrderStatus::AllCanceled;
		onOrderMsg.todayDealedVol = 0.0;

	}

	PushOnOrderMsgs(onOrderMsg);
	return;

}

