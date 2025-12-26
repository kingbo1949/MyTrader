#include "pch.h"
#include "OnRtn_ErrCancel.h"
#include <Factory_Log.h>
#include <Factory_Topics.h>
#include <Factory_IBJSon.h>
COnRtn_ErrCancel::COnRtn_ErrCancel(OneOrderPtr pOriginalOrder)
	:m_pOriginalOrder(pOriginalOrder)
{

}

void COnRtn_ErrCancel::Execute()
{
	if (!m_pOriginalOrder) return;

	RemoveActiveAction(m_pOriginalOrder->orderKey.strategyIdHashId, m_pOriginalOrder->orderKey.stSubModule, m_pOriginalOrder->localOrderNo);

	Log_Print(LogLevel::Warn, "call in COnRtn_ErrCancel");

	// 驱动应用层
	if (MakeAndGet_JSonBroker()->LoadBroker()->setupType == SetupType::RealTrader)
	{
		MakeAndGet_Topic_OnTrade()->UpdateTick_OnTrade(m_pOriginalOrder->orderKey.strategyIdHashId);
	}
	

	return;

}
