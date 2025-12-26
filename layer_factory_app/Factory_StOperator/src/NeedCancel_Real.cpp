#include "pch.h"
#include "NeedCancel_Real.h"
#include <Factory_Log.h>
#include <Factory_HashEnv.h>
#include <Factory_UnifyInterface.h>
#include "Factory_StOperator.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>

CNeedCancel_Real::CNeedCancel_Real(const SubModuleParams& stParams)
	:m_stParams(stParams)
{
	m_pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(m_stParams.strategyIdHashId);
	if (!m_pStrategyParam)
	{
		Log_Print(LogLevel::Err, "Get_StrategyParam return null");
		exit(-1);
	}
	m_targetCodeIdEnv = MakeAndGet_QEnvManager()->GetEnvOneCode(m_stParams.targetCodeId)->GetTicEnv();
}

bool CNeedCancel_Real::CanCancel()
{
	if (Get_Sys_Status() == Thread_Status::Stop) return true;

	bool success = false;
	if (m_stParams.openOrCover == RoundAction::Open_Round)
	{
		success = CanCancelOpen();
	}
	else
	{
		success = CanCancelCover();
	}
	return success;

}

bool CNeedCancel_Real::CheckRecentOrderOnTop(int second)
{
	OneOrderPtr porder = GetActiveOrder(m_stParams.strategyIdHashId, m_stParams.subModule);

	if (Get_CurrentTime()->GetCurrentTime_millisecond() - porder->tickTime < second * 1000)
	{
		// 挂单不到second秒
		return true;
	}

	int priceStep = CGlobal::DToI(porder->orderPriceReal / m_stParams.contract->minMove);
	int priceStatus = CHighFrequencyGlobalFunc::GetPriceStatus(m_targetCodeIdEnv->pTick, m_stParams.buyOrSell, priceStep);
	if (priceStatus == 0 || priceStatus == 1)
	{
		// 挂单在最前面
		return true;
	}

	return false;
}
