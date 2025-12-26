#include "pch.h"
#include "StrategySuit.h"
#include "Strategy_Open.h"
#include "Strategy_Cover.h"
CStrategySuit::CStrategySuit(StrategyParamPtr strategyParam)
	:m_strategyParam(strategyParam)
{

}

void CStrategySuit::MakeSuit()
{
	if (m_strategyParam->canOpen)
	{
		StrategyBasePtr stOpen = std::make_shared<CStrategy_Open>(m_strategyParam);
		stOpen->Init();
		m_subModules.push_back(stOpen);

	}
	if (m_strategyParam->canCover)
	{
		StrategyBasePtr stCover = std::make_shared<CStrategy_Cover>(m_strategyParam);
		stCover->Init();
		m_subModules.push_back(stCover);

	}

	return ;
	
}

StrategyBasePtrs CStrategySuit::GetStrategySumModules()
{
	return m_subModules;
}
