#include "pch.h"
#include "OnRtn_Order.h"
#include <Factory_TShareEnv.h>
#include <Factory_Topics.h>
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>
#include <Global.h>
COnRtn_Order::COnRtn_Order(const OnOrderMsg& msg)
	:m_msg(msg)

{

}

void COnRtn_Order::Execute()
{
	if (!m_msg.pOriginalOrder) return;
	//Log_Print(LogLevel::Info, fmt::format("localOrderNo = {}, COnRtn_Order execute, status = {}, todaySumDealedQuantity = {}",
	//	m_pOriginalOrder->localOrderNo,
	//	CTransToStr::Get_OrderStatus(m_orderStatus).c_str(),
	//	m_todaySumDealedQuantity
	//));
	RemoveActiveAction(m_msg.pOriginalOrder->orderKey.strategyIdHashId, m_msg.pOriginalOrder->orderKey.stSubModule, m_msg.pOriginalOrder->localOrderNo);

	m_msg.pOriginalOrder->orderStatus = m_msg.orderStatus;
	m_msg.pOriginalOrder->dealedQuantity = m_msg.todayDealedVol;
	m_msg.pOriginalOrder->avgDealedPrice = m_msg.todayDealedAvgPrice;
	if (m_msg.pOriginalOrder->orderStatus == OrderStatus::AllDealed || m_msg.pOriginalOrder->orderStatus == OrderStatus::AllCanceled)
	{
		MoveOrderFromActiveToInactive(m_msg.pOriginalOrder->orderKey, m_msg.pOriginalOrder->localOrderNo);
	}
	if (m_msg.pOriginalOrder->orderStatus == OrderStatus::AllDealed)
	{
		MakeAndGet_DbLastOrder()->UpdateLastOrderDealTime(m_msg.pOriginalOrder->orderKey, Get_CurrentTime()->GetCurrentTime_millisecond());
		MakeAndGet_DbLastOrder()->UpdateLastOrderNo(m_msg.pOriginalOrder->orderKey, m_msg.pOriginalOrder->localOrderNo);
		if (m_msg.pOriginalOrder->openOrCover == RoundAction::Cover_Round)
		{
			size_t cover_count = MakeAndGet_DbLastOrder()->GetCoverCount(m_msg.pOriginalOrder->orderKey.strategyIdHashId);
			MakeAndGet_DbLastOrder()->UpdateCoverCount(m_msg.pOriginalOrder->orderKey.strategyIdHashId, cover_count + 1);
		}
	}
	

	// 驱动应用层
	//if (MakeAndGet_JSonBroker()->LoadBroker()->setupType == SetupType::RealTrader)
	//{
	//	MakeAndGet_Topic_OnTrade()->UpdateTick_OnTrade(m_msg.pOriginalOrder->orderKey.strategyIdHashId);
	//}


	return;

}