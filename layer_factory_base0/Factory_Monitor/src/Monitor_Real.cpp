#include "pch.h"
#include "Monitor_Real.h"
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>

void CMonitor_Real::Clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_recentExec.clear();

}

void CMonitor_Real::PrintRecentExec()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto oneitem : m_recentExec)
	{
		Log_Print(LogLevel::Info, fmt::format("\n\t{} {} {}", 
			oneitem.first.strategyId.c_str(), 
			CTransToStr::Get_RoundAction(oneitem.first.openOrCover).c_str(),
			oneitem.second.c_str() 
		
		));
	}
}

void CMonitor_Real::UpdateRecentExec(const StrategyId& strategyId, RoundAction openOrCover, const std::string& dec)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	MonitorKey key;
	key.strategyId = strategyId;
	key.openOrCover = openOrCover;
	m_recentExec[key] = "\n\t" + dec;
	return;
}
