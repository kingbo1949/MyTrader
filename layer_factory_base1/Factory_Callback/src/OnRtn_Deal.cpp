#include "pch.h"
#include "OnRtn_Deal.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include <Factory_Log.h>
#include <Global.h>
#include <Factory_IBJSon.h>
#include <Factory_Topics.h>
COnRtn_Deal::COnRtn_Deal(const OnTradeMsg& msg)
	:m_msg(msg)
{

}

void COnRtn_Deal::Execute()
{
	if (!m_msg.pOriginalOrder) return;

	// 填写deal字段
	OneDealPtr pDeal = MakeDeal();

	// 加入数据库
	MakeAndGet_DbDeal()->AddOne(pDeal);

	// 修改持仓
	MakeAndGet_DbPositionSize()->NewDeal(m_msg);

	// 驱动应用层
	//if (MakeAndGet_JSonBroker()->LoadBroker()->setupType == SetupType::RealTrader)
	//{
	//	MakeAndGet_Topic_OnTrade()->UpdateTick_OnTrade(m_msg.pOriginalOrder->orderKey.strategyIdHashId);
	//}

	return;

}

OneDealPtr COnRtn_Deal::MakeDeal()
{
	// dealNo dealQuantity dealPrice 在外部已经被填写
	OneDealPtr pDeal = std::make_shared<COneDeal>();
	pDeal->dealNo = m_msg.dealNo;
	pDeal->dealPrice = m_msg.dealPrice;
	pDeal->dealQuantity = m_msg.dealQuantity;

	pDeal->key.strategyIdHashId = m_msg.pOriginalOrder->orderKey.strategyIdHashId;
	pDeal->key.stSubModule = m_msg.pOriginalOrder->orderKey.stSubModule;
	pDeal->codeHashId = m_msg.pOriginalOrder->codeHashId;
	pDeal->localOrderNo = m_msg.pOriginalOrder->localOrderNo;
	pDeal->buyOrSell = m_msg.pOriginalOrder->buyOrSell;
	pDeal->openOrCover = m_msg.pOriginalOrder->openOrCover;

	pDeal->dealIndex = m_msg.pOriginalOrder->tickIndex;
	pDeal->dealTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();
	return pDeal;


}

