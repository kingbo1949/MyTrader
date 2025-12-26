#include "pch.h"
#include "MakePrice_JumpTick.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Factory_TShareEnv.h>
#include <Factory_Log.h>
#include <Global.h>

CMakePrice_JumpTick::CMakePrice_JumpTick(const SubModuleParams& stParams)
	:CMakePrice_Real(stParams)
{
}

bool CMakePrice_JumpTick::GetOpenPrice(double& price, RealPriceType& priceType)
{
	if (Get_Sys_Status() != Thread_Status::Running) return false;
	
	int minJumpStep = 8;	// 至少需要的价格跃迁
	if (m_stParams.buyOrSell == BuyOrSell::Buy)
	{
		if (m_targetTickEnv->pTick->bidAsks[0].bid - m_targetLastTickEnv->pTick->bidAsks[0].bid < minJumpStep) return false;

		// tick暴涨，开始挂单
		price = m_targetLastTickEnv->pTick->bidAsks[0].bid * m_stParams.contract->minMove;
		priceType = RealPriceType::Original;

	}
	else
	{
		if (m_targetLastTickEnv->pTick->bidAsks[0].ask  - m_targetTickEnv->pTick->bidAsks[0].ask < minJumpStep) return false;

		// tick暴跌，开始挂单
		price = m_targetLastTickEnv->pTick->bidAsks[0].ask * m_stParams.contract->minMove;
		priceType = RealPriceType::Original;

	}
	return true;
}

bool CMakePrice_JumpTick::GetCoverPrice(double& price, RealPriceType& priceType)
{
	std::string timestr = CGlobal::GetTickTimeStr(Now());

	// 得到开仓单成交时间
	time_t lastOpenDealTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));

	LocalOrderNo localorderNo = MakeAndGet_DbLastOrder()->GetLastOrderNo(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));
	OneOrderPtr pOpenOrder = MakeAndGet_DbInactiveOrder()->GetOne(localorderNo);
	if (!pOpenOrder)
	{
		Log_Print(LogLevel::Err, "pOpenOrder is null");
		return false;
	}
	// 开仓单一成交就会下入平仓单
	// 开仓单刚刚成交不久，则判断是第一次下平仓单，否则是平仓单不成交撤单后止损单
	if (Now() - lastOpenDealTime < 500)
	{
		return GetCoverPrice_AfterOpen(pOpenOrder, price, priceType);
	}
	else
	{
		return GetCoverPrice_AfterCancel(price, priceType);
	}

}

bool CMakePrice_JumpTick::GetCoverPrice_AfterOpen(OneOrderPtr pOpenOrder, double& price, RealPriceType& priceType)
{
	// 按照盈利价位平仓
	int priceStep = 12;
	if (pOpenOrder->buyOrSell == BuyOrSell::Buy)
	{
		price = pOpenOrder->orderPriceStrategy + priceStep * m_stParams.contract->minMove;
	}
	else
	{
		price = pOpenOrder->orderPriceStrategy - priceStep * m_stParams.contract->minMove;
	}
	priceType = RealPriceType::Original;
	return true;
}

bool CMakePrice_JumpTick::GetCoverPrice_AfterCancel(double& price, RealPriceType& priceType)
{
	priceType = RealPriceType::Original;
	price = CHighFrequencyGlobalFunc::GetPriceByMidPoint(m_targetTickEnv->pTick, m_stParams.buyOrSell) * m_stParams.contract->minMove;
	return true;
}
