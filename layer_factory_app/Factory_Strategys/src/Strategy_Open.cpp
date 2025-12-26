#include "pch.h"
#include "Strategy_Open.h"
#include <Factory_HashEnv.h>
#include <Factory_TShareEnv.h>
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <base_trade.h>
#include <Global.h>
CStrategy_Open::CStrategy_Open(StrategyParamPtr strategyParam)
	:CStrategy_Real(strategyParam)
{

}

void CStrategy_Open::SetTradeParams()
{
	m_stParams.targetCodeId = Get_CodeIdEnv()->Get_CodeId_Hash(m_strategyParam->key.targetCodeId.c_str());
	m_stParams.contract = MakeAndGet_ContractEnv()->GetContract(m_stParams.targetCodeId);
	m_stParams.strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(m_strategyParam->key.ToStr());
	m_stParams.openOrCover = RoundAction::Open_Round;
	m_stParams.buyOrSell = m_strategyParam->key.buyOrSell;
	m_stParams.subModule = StSubModule::SubOpen;
	return;
}

double CStrategy_Open::MakeOrderVol()
{
	//string nowtimestr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	//printf("in OPEN , nowtimestr = %s \n", nowtimestr.c_str());


	// 按探针手数下单开仓，直到达到maxVol
	PositionSizePtr positionSize = MakeAndGet_DbPositionSize()->GetOne(m_stParams.strategyIdHashId);
	if (positionSize)
	{
		if (positionSize->vol >= m_strategyParam->maxVol) return 0.0;

		double canOpenVol = m_strategyParam->maxVol - positionSize->vol;
		if (canOpenVol >= m_strategyParam->probeVol) return m_strategyParam->probeVol;

		return canOpenVol;
	}
	else
	{
		return m_strategyParam->probeVol;
	}

	
}

