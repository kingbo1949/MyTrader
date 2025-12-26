#include "pch.h"
#include "BaseLine.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include <Factory_HashEnv.h>
CBaseLine::CBaseLine(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint)
	:m_pStrategyParam(pStrategyParam), m_pContract(pContract), m_openPoint(openPoint)
{
	m_strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(m_pStrategyParam->key.ToStr());

	// 从数据库中取pricePair对象实例，经过这一步m_pricePair永远不会为空值
	m_pricePair = MakeAndGet_DbPricePairs()->GetPricePair(m_strategyIdHashId);
}

PricePairPtr CBaseLine::MakeAndGetLine(time_t now, RoundAction roundAction)
{
	if (roundAction == RoundAction::Cover_Round)
	{
		return m_pricePair;
	}


	if (!NeedFreshPricePair())
	{
		return m_pricePair;
	}

	if (FreshPricePair(now))
	{
		return m_pricePair;
	}
	else
	{
		return nullptr;
	}

}


CodeStr CBaseLine::GetCodeId()
{
	return m_pStrategyParam->key.targetCodeId;
}

Time_Type CBaseLine::GetTimeType()
{
	return m_pStrategyParam->timetype;
}


