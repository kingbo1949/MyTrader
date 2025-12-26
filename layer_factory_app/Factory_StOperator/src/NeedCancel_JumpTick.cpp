#include "pch.h"
#include "NeedCancel_JumpTick.h"
#include <Factory_TShareEnv.h>
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QShareEnv.h>
#include <Global.h>

CNeedCancel_JumpTick::CNeedCancel_JumpTick(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_JumpTick::CanCancelOpen()
{
	// N秒挂单不在最前面则撤单
	int second = 5;
	if (CheckRecentOrderOnTop(second))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool CNeedCancel_JumpTick::CanCancelCover()
{
	int second = 5;
	if (!CheckRecentOrderOnTop(second))
	{
		// 平仓单已经等待了足够的时间，且不在盘口当中，撤单
		return true;
	}

	std::string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());

	// 得到开仓单成交时间
	time_t lastOpenDealTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));

	LocalOrderNo localorderNo = MakeAndGet_DbLastOrder()->GetLastOrderNo(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));
	OneOrderPtr pOpenOrder = MakeAndGet_DbInactiveOrder()->GetOne(localorderNo);
	if (!pOpenOrder)
	{
		Log_Print(LogLevel::Err, "pOpenOrder is null");
		return false;
	}

	
	if (Get_CurrentTime()->GetCurrentTime_millisecond() - lastOpenDealTime < 1500) return false;

	IBTickPtr ptick = MakeAndGet_QEnvManager()->GetEnvOneCode(m_stParams.targetCodeId)->GetTicEnv()->pTick;
	double minMove = m_stParams.contract->minMove;

	std::string tickstr = fmt::format("{:.2f}->{:.2f}", ptick->bidAsks[0].bid * minMove, ptick->bidAsks[0].ask * minMove);
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Buy && pOpenOrder->orderPriceStrategy > ptick->bidAsks[0].ask * minMove)
	{
		// 处于亏损状态 撤单
		return true;
	}
	if (m_pStrategyParam->key.buyOrSell == BuyOrSell::Sell && pOpenOrder->orderPriceStrategy < ptick->bidAsks[0].bid * minMove)
	{
		// 处于亏损状态 撤单
		return true;
	}
	return false;





}
