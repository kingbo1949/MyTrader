#include "pch.h"
#include "IBApi_Real.h"
#include <Factory_Log.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include "Factory_UnifyInterface.h"


void CIBApi_Real::Release()
{
	Set_Sys_Status(Thread_Status::Stop);

	// 因为IB的守护线程在无行情的非开盘状态，2秒才会被触发一次，才能发现系统要退出，因此主线程休眠3秒
	std::this_thread::sleep_for(std::chrono::seconds(3));
	return;
}

bool CIBApi_Real::SendOrder(OneOrderPtr porder)
{
	// 添加action
	ActionPtr pAction = std::make_shared<CAction>();
	pAction->key.strategyIdHashId = porder->orderKey.strategyIdHashId;
	pAction->key.stSubModule = porder->orderKey.stSubModule;
	pAction->localOrderNo = porder->localOrderNo;
	pAction->actionType = ActionType::SendOrder;
	MakeAndGet_DbActiveAction()->AddOne(pAction);

	// 添加活跃委托
	MakeAndGet_DbActiveOrder()->AddOne(porder);
	
	return true;
}

bool CIBApi_Real::CancelOrder(OneOrderPtr porder, const std::string& manualOrderCancelTime)
{
	// 添加action
	ActionPtr pAction = std::make_shared<CAction>();
	pAction->key.strategyIdHashId = porder->orderKey.strategyIdHashId;
	pAction->key.stSubModule = porder->orderKey.stSubModule;
	pAction->localOrderNo = porder->localOrderNo;
	pAction->actionType = ActionType::CancelOrder;
	MakeAndGet_DbActiveAction()->AddOne(pAction);

	return true;
}



