#include "pch.h"
#include "db_LastOrder_Real.h"
#include <Factory_HashEnv.h>
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>

Cdb_LastOrder_Real::Cdb_LastOrder_Real()
	:m_status(FileStatus::Saved)
{
}

FileStatus Cdb_LastOrder_Real::GetStatus()
{
	return m_status;
}

void Cdb_LastOrder_Real::SetStatus(FileStatus fileStatus)
{
	m_status = fileStatus;
}

void Cdb_LastOrder_Real::UpdateLastOrderNo(const OrderKey& orderkey, LocalOrderNo orderNo)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	m_status = FileStatus::Updated;
	m_lastOrderNos[orderkey] = orderNo;
	
}

void Cdb_LastOrder_Real::UpdateLastOrderDealTime(const OrderKey& orderkey, Tick_T ms)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	m_status = FileStatus::Updated;
	m_lastOrderDealTimes[orderkey] = ms;
}

Tick_T Cdb_LastOrder_Real::GetLastOrderDealTime(const OrderKey& orderkey)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::map<OrderKey, Tick_T>::iterator pos = m_lastOrderDealTimes.find(orderkey);
	if (pos == m_lastOrderDealTimes.end()) return 0;

	return pos->second;
}

LocalOrderNo Cdb_LastOrder_Real::GetLastOrderNo(const OrderKey& orderkey)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::map<OrderKey, LocalOrderNo>::iterator pos = m_lastOrderNos.find(orderkey);
	if (pos == m_lastOrderNos.end()) return 0;

	return pos->second;
}


void Cdb_LastOrder_Real::PrintAll()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (const auto& one : m_lastOrderDealTimes)
	{
		StrategyParamPtr pParam = Get_StrategyParamEnv()->Get_StrategyParam(one.first.strategyIdHashId);
		Log_Print(LogLevel::Info, fmt::format("lastOrderDealTimes {} {} {}",
			pParam->key.ToStr().c_str(),
			CTransToStr::Get_StSubModule(one.first.stSubModule).c_str(),
			CGlobal::GetTickTimeStr(one.second).c_str()));
	}
	for (const auto& one : m_lastOrderNos)
	{
		StrategyParamPtr pParam = Get_StrategyParamEnv()->Get_StrategyParam(one.first.strategyIdHashId);
		Log_Print(LogLevel::Info, fmt::format("      lastOrderNos {} {} {}",
			pParam->key.ToStr().c_str(),
			CTransToStr::Get_StSubModule(one.first.stSubModule).c_str(),
			one.second));
	}
	for (const auto& one : m_coverCount)
	{
		StrategyParamPtr pParam = Get_StrategyParamEnv()->Get_StrategyParam(one.first);
		Log_Print(LogLevel::Info, fmt::format("        coverCount {} {}",
			pParam->key.ToStr().c_str(),
			one.second));

	}


}

size_t Cdb_LastOrder_Real::GetCoverCount(StrategyIdHashId id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::map<StrategyIdHashId, size_t>::iterator pos = m_coverCount.find(id);
	if (pos == m_coverCount.end()) return 0;

	return pos->second;
}

void Cdb_LastOrder_Real::UpdateCoverCount(StrategyIdHashId id, size_t count)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	m_status = FileStatus::Updated;
	m_coverCount[id] = count;
	return;
}

void Cdb_LastOrder_Real::Clear(StrategyIdHashId id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_status = FileStatus::Updated;
	//string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());

	for (auto& item : m_lastOrderDealTimes)
	{
		if (item.first.strategyIdHashId == id)
		{
			item.second = 0;
		}
	}
	for (auto& item : m_lastOrderNos)
	{
		if (item.first.strategyIdHashId == id)
		{
			item.second = 0;
		}
	}
	for (auto& item : m_coverCount)
	{
		if (item.first == id)
		{
			item.second = 0;
		}
	}
	return;
}

void Cdb_LastOrder_Real::GetAll(std::map<OrderKey, Tick_T>& lastOrderDealTimes, std::map<OrderKey, LocalOrderNo>& lastOrderNos, std::map<StrategyIdHashId, size_t>& coverCounts)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	lastOrderDealTimes = m_lastOrderDealTimes;
	lastOrderNos = m_lastOrderNos;
	coverCounts = m_coverCount;
	return;
}
