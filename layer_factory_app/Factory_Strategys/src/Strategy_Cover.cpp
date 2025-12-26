#include "pch.h"
#include "Strategy_Cover.h"
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Factory_TShareEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
CStrategy_Cover::CStrategy_Cover(StrategyParamPtr strategyParam)
	:CStrategy_Real(strategyParam)
{

}

void CStrategy_Cover::SetTradeParams()
{
	m_stParams.targetCodeId = Get_CodeIdEnv()->Get_CodeId_Hash(m_strategyParam->key.targetCodeId.c_str());
	m_stParams.contract = MakeAndGet_ContractEnv()->GetContract(m_stParams.targetCodeId);
	m_stParams.strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(m_strategyParam->key.ToStr());
	m_stParams.openOrCover = RoundAction::Cover_Round;
	m_stParams.buyOrSell = CReverse::Reverse_BuyOrSell(m_strategyParam->key.buyOrSell) ;
	m_stParams.subModule = StSubModule::SubCover;
	return;

}

double CStrategy_Cover::MakeOrderVol()
{
	//string nowtimestr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	//printf("in COVER , nowtimestr = %s \n", nowtimestr.c_str());

	PositionSizePtr positionSize = MakeAndGet_DbPositionSize()->GetOne(m_stParams.strategyIdHashId);
	if (!positionSize) return 0;

	return positionSize->vol;
}
