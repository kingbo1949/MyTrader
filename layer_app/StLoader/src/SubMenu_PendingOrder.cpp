#include "SubMenu_PendingOrder.h"


#include <iostream>
#include <string>
#include <Factory_IBJSon.h>
#include <Factory_Log.h>
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Factory_QDatabase.h>
#include <Factory_TShareEnv.h>
#include <Global.h>


void CSubMenu_PendingOrder::ShowMenu()
{
	std::cout <<
		"usage:\n"
		"s: send pending test order without quote \n"
		"c: cancel pending test order without quote \n"
		"x: exit\n";

}

bool CSubMenu_PendingOrder::HandleReceive(const std::string& cmd)
{
	if (cmd == "s" || cmd == "S")
	{
		SendPendingTestOrder();
		return true;
	}
	else if (cmd == "c" || cmd == "C")
	{
		CancelPendingTestOrder();
		return true;
	}
	else
	{
		return false;
	}
}

void CSubMenu_PendingOrder::SendPendingTestOrder()
{
	StrategyParamPtr pStrategyParam = GetStrategyParamForPendingTestOrder();
	if (!pStrategyParam) return;

	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(pStrategyParam->key.targetCodeId.c_str());

	OneOrderPtr porder = std::make_shared<COneOrder>();

	porder->orderKey.strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(pStrategyParam->key.ToStr());
	porder->orderKey.stSubModule = StSubModule::SubOpen;

	porder->codeHashId = Get_CodeIdEnv()->Get_CodeId_Hash(pStrategyParam->key.targetCodeId.c_str());
	porder->localOrderNo = int(MakeAndGet_SeriesNoMaker()->GetSeriesNo());

	porder->tickIndex = 0;
	porder->tickTime = Get_CurrentTime()->GetCurrentTime_millisecond();
	porder->openOrCover = RoundAction::Open_Round;
	porder->buyOrSell = pStrategyParam->key.buyOrSell;
	porder->orderStatus = OrderStatus::SomeDealed;

	porder->orderQuantity = 1;
	porder->orderPriceReal = 18000;
	porder->orderPriceStrategy = 18000;

	if (contract->securityType == SecurityType::FUT && CGlobal::DToI(porder->orderQuantity) != 1)
	{
		Log_Print(LogLevel::Warn, "want send a PendingTest order with too big Quantity");
		return;
	}

	Get_IBApi()->SendOrder(porder);
	Log_Print(LogLevel::Warn, "send a PendingTest order");
	return;

}

StrategyParamPtr CSubMenu_PendingOrder::GetStrategyParamForPendingTestOrder()
{
	// 取得准备手动发送的测试策略 买开仓
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);

	StrategyParamPtr pStrategyParam = nullptr;
	for (auto strategy_param : strategyParams)
	{
		if (strategy_param->key.strategyName == "manual" && strategy_param->key.buyOrSell == BuyOrSell::Buy)
		{
			pStrategyParam = strategy_param;
			break;
		}
	}
	if (!pStrategyParam)
	{
		Log_Print(LogLevel::Warn, "can not find manual_buy strategy");
		return nullptr;
	}
	return pStrategyParam;
}

void CSubMenu_PendingOrder::CancelPendingTestOrder()
{
	StrategyParamPtr pStrategyParam = GetStrategyParamForPendingTestOrder();
	if (!pStrategyParam) return;

	OrderKey key;
	key.strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(pStrategyParam->key.ToStr());
	key.stSubModule = StSubModule::SubOpen;
	OneOrderPtrs orders = MakeAndGet_DbActiveOrder()->GetAll(key);
	for (auto oneorder : orders)
	{
		Get_IBApi()->CancelOrder(oneorder, "");
		Log_Print(LogLevel::Warn, "cancel a PendingTest order");
	}
	return;

}
