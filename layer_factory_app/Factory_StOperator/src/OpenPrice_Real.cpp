#include "pch.h"
#include "OpenPrice_Real.h"

bool COpenPrice_Real::GetOpenPrice(StrategyIdHashId strategyIdHashId, double& price)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::map<StrategyIdHashId, double>::const_iterator pos = m_openPrices.find(strategyIdHashId);
	if (pos == m_openPrices.end()) return false;

	price = pos->second;
	return true;

}

void COpenPrice_Real::SetOpenPrice(StrategyIdHashId strategyIdHashId, double price)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_openPrices[strategyIdHashId] = price;
	return;
}

void COpenPrice_Real::DelOpenPrice(StrategyIdHashId strategyIdHashId)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::map<StrategyIdHashId, double>::iterator pos = m_openPrices.find(strategyIdHashId);
	if (pos == m_openPrices.end()) return ;

	m_openPrices.erase(pos);
	return;
}
