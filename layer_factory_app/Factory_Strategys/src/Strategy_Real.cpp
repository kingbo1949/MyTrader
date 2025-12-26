#include "pch.h"
#include "Strategy_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include <Factory_QShareEnv.h>
#include <Factory_UnifyInterface.h>
#include <Factory_HashEnv.h>
#include <Factory_Log.h>
#include <Factory_IBJSon.h>
#include <Global.h>
#include <Factory_QDatabase.h>

CStrategy_Real::CStrategy_Real(StrategyParamPtr	strategyParam)
	:m_strategyParam(strategyParam)
{
	CodeHashId targetCode = Get_CodeIdEnv()->Get_CodeId_Hash(m_strategyParam->key.targetCodeId.c_str());
	m_tickEnv = MakeAndGet_QEnvManager()->GetEnvOneCode(targetCode)->GetTicEnv();
}

void CStrategy_Real::OnRtnTick(CodeHashId codeHash, RoundAction openOrCover)
{
	if (openOrCover != m_stParams.openOrCover) return;

	if (m_stParams.strategyIdHashId == 0)
	{
		Log_Print(LogLevel::Err, "m_stParams.strategyIdHashId == 0");
		exit(-1);
	}

	if (codeHash != m_stParams.targetCodeId) return;



	Execute();
	return;
}


void CStrategy_Real::OnRtnTrade(StrategyIdHashId strategyIdHashId)
{
	if (strategyIdHashId != m_stParams.strategyIdHashId) return;

	Execute();
	return;

}


void CStrategy_Real::Init()
{
	SetTradeParams();

	if (!m_pMakePrice)
	{
		m_pMakePrice = MakeAndGet_MakePrice(m_stParams);
	}

	return;
}

void CStrategy_Real::Execute()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!CheckTickEnv()) return;

	if (GetStHalfStatus() != StHalfStatus::AtFirst)
	{
		// 处于half态，不可继续执行
		return;
	}

	StModuleStatus moduleStatus = GetStModuleStatus();
	if (moduleStatus == StModuleStatus::DoAtFirst)
	{
		// 检查是否要发单
		Execute_DoAtFirst();
	}
	else
	{
		// 已经发送了委托，检查成交情况及是否要撤单
		Execute_DoSend();
	}
	return;


}


void CStrategy_Real::Execute_DoAtFirst()
{
	// 开始准备下单
	double vol = MakeOrderVol();
	if (CGlobal::IsZero(vol)) return;

	double priceStrategy = 0.0;
	RealPriceType priceType = RealPriceType::Original;
	if (!MakeOrderPrice(priceStrategy, priceType)) return;

	double priceReal = MakeSendPrice(priceStrategy, priceType);

	// 开始制作委托
	OneOrderPtr porder = MakeOrder(vol, priceStrategy, priceReal);
	if (!porder) return;

	// 发送
	SendOrder(porder);
	return;
}

void CStrategy_Real::Execute_DoSend()
{
	// 检查是否需要撤单
	if (!NeedCancel()) return;

	// 开始准备撤单

	OneOrderPtr porder = GetActiveOrder(m_stParams.strategyIdHashId, m_stParams.subModule);
	if (!porder)
	{
		Log_Print(LogLevel::Warn, fmt::format("wanto to cancel order, there is no activeorder, {} subModule = {}",
			Get_StrategyParamEnv()->Get_StrategyParam(porder->orderKey.strategyIdHashId)->key.ToStr().c_str(),
			CTransToStr::Get_StSubModule(m_stParams.subModule).c_str()
			) );
		return;
	}

	// 发送撤单
	CancelOrder(porder);
	
	Log_Print(LogLevel::Info, fmt::format("send cancel order, {} subModule = {}, tickIndex = {}, {}",
		Get_StrategyParamEnv()->Get_StrategyParam(porder->orderKey.strategyIdHashId)->key.ToStr().c_str(),
		CTransToStr::Get_StSubModule(m_stParams.subModule).c_str(),
		m_tickEnv->tickIndex,
		CGlobal::GetTickTimeStr(m_tickEnv->pTick->time).c_str()

	));


	return;


}

StModuleStatus CStrategy_Real::GetStModuleStatus()
{
	// 本策略模块有活跃未成交单，则处于DoSend状态，否则处于DoAtFirst态
	OrderKey key;
	key.strategyIdHashId = m_stParams.strategyIdHashId;
	key.stSubModule = m_stParams.subModule;
	OneOrderPtrs orders = MakeAndGet_DbActiveOrder()->GetAll(key);

	if (orders.size() == 0) return StModuleStatus::DoAtFirst;
	return StModuleStatus::DoSend;
}

StHalfStatus CStrategy_Real::GetStHalfStatus()
{
	// 本模块中是否有活跃的action
	ActionKey key;
	key.strategyIdHashId = m_stParams.strategyIdHashId;
	key.stSubModule = m_stParams.subModule;
	ActionPtrs actions = MakeAndGet_DbActiveAction()->GetAll(key);

	for (auto oneaction : actions)
	{
		if (oneaction->actionType == ActionType::CancelOrder) return StHalfStatus::DoCancelOrderHalf;
		if (oneaction->actionType == ActionType::SendOrder) return StHalfStatus::DoSendOrderHalf;
	}

	return StHalfStatus::AtFirst;
}

bool CStrategy_Real::MakeOrderPrice(double& price, RealPriceType& priceType)
{
	if (!m_pMakePrice)
	{
		m_pMakePrice = MakeAndGet_MakePrice(m_stParams);
	}
	return m_pMakePrice->GetPrice(price, priceType);


}

double CStrategy_Real::MakeSendPrice(double priceStrategy, RealPriceType priceType)
{
	double priceReal = 0.0;
	if (MakeAndGet_JSonBroker()->LoadBroker()->setupType == SetupType::Simulator)
	{
		// 模拟价格发单，直接使用策略生成的价格
		return priceStrategy;
	}

	if (priceType == RealPriceType::Original) return priceStrategy;
	if (priceType == RealPriceType::Wait)
	{
		// 0号位挂单
		priceReal = CHighFrequencyGlobalFunc::GetPriceByPriceStatus(m_tickEnv->pTick, m_stParams.buyOrSell, 0) * m_stParams.contract->minMove;
		return priceReal;
	}
	if (priceType == RealPriceType::Eat)
	{
		// 对价吃
		priceReal = CHighFrequencyGlobalFunc::GetPriceByPriceStatus(m_tickEnv->pTick, m_stParams.buyOrSell, -80) * m_stParams.contract->minMove;
		PrintPriceToLog(priceStrategy, priceReal);
		return priceReal;
	}
	if (priceType == RealPriceType::MidPoint)
	{
		
		// 中间价挂单，如果买卖盘之间只相差一个价位，则挂在0号位
		priceReal = CHighFrequencyGlobalFunc::GetPriceByMidPoint(m_tickEnv->pTick, m_stParams.buyOrSell) * m_stParams.contract->minMove;
		return priceReal;
	}
	return priceReal;
}

bool CStrategy_Real::NeedCancel()
{
	if (!m_pNeedCancel)
	{
		m_pNeedCancel = MakeAndGet_NeedCancel(m_stParams);
	}

	return m_pNeedCancel->CanCancel();


}

OneOrderPtr CStrategy_Real::MakeOrder(double vol, double priceStrategy, double priceReal)
{
	OneOrderPtr porder = std::make_shared<COneOrder>();

	porder->orderKey.strategyIdHashId = m_stParams.strategyIdHashId;
	porder->orderKey.stSubModule = m_stParams.subModule;

	porder->codeHashId = m_stParams.targetCodeId;
	porder->localOrderNo = int(MakeAndGet_SeriesNoMaker()->GetSeriesNo());

	porder->tickIndex = m_tickEnv->tickIndex;
	porder->tickTime = m_tickEnv->pTick->time;
	porder->openOrCover = m_stParams.openOrCover;
	porder->buyOrSell = m_stParams.buyOrSell;
	porder->orderStatus = OrderStatus::SomeDealed;

	porder->orderQuantity = vol;
	porder->orderPriceStrategy = priceStrategy;
	porder->orderPriceReal = priceReal;
	

	return porder;

}

bool CStrategy_Real::SendOrder(OneOrderPtr porder)
{
	Log_Print(LogLevel::Warn, fmt::format("tickIndex = {}, {} {} {} vol = {}, priceStrategy = {:.2f}, priceReal = {:.2f}, {:.2f}->{:.2f}, {} {}",
		m_tickEnv->tickIndex,
		CGlobal::GetTickTimeStr(m_tickEnv->pTick->time).c_str(),
		CTransToStr::Get_BuyOrSell(porder->buyOrSell).c_str(),
		CTransToStr::Get_RoundAction(porder->openOrCover).c_str(),
		porder->orderQuantity,
		porder->orderPriceStrategy,
		porder->orderPriceReal,
		m_tickEnv->pTick->bidAsks[0].bid * m_stParams.contract->minMove,
		m_tickEnv->pTick->bidAsks[0].ask * m_stParams.contract->minMove,
		Get_CodeIdEnv()->Get_CodeStrByHashId(porder->codeHashId),
		Get_StrategyParamEnv()->Get_StrategyParam(porder->orderKey.strategyIdHashId)->key.ToStr().c_str()
		));

	return Get_IBApi()->SendOrder(porder);
	

}

bool CStrategy_Real::CancelOrder(OneOrderPtr porder)
{
	Get_IBApi()->CancelOrder(porder, "");
	return true;
}

bool CStrategy_Real::CheckTickEnv()
{
	TickEnvPtr targetLastTickEnv = MakeAndGet_QEnvManager()->GetEnvOneCode(m_stParams.targetCodeId)->GetLastTicEnv();
	if (!targetLastTickEnv->pTick)
	{
		Log_Print(LogLevel::Warn, fmt::format("{} last tick is null", m_stParams.contract->codeId.c_str() ));
		return false;
	}
	return true;

}

void CStrategy_Real::PrintPriceToLog(double strategyPrice, double realSendPrice)
{
	std::string temstr = fmt::format("will send order, strategy price: {:.2f}, real price = {:.2f}, {:.2f}->{:.2f}",
		strategyPrice, realSendPrice,
		m_tickEnv->pTick->bidAsks[0].bid * m_stParams.contract->minMove,
		m_tickEnv->pTick->bidAsks[0].ask * m_stParams.contract->minMove
		);
	int modSecond = 1;
	Log_AsyncPrintRotatingFile("orderprice.log", temstr, modSecond, false);

}

