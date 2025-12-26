#include "pch.h"
#include "db_PricePair_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <Factory_HashEnv.h>
#include <Factory_Log.h>

Cdb_PricePair_Real::Cdb_PricePair_Real()
	:m_status(FileStatus::Saved)
{
}

FileStatus Cdb_PricePair_Real::GetStatus()
{
	return m_status;
}

void Cdb_PricePair_Real::SetStatus(FileStatus fileStatus)
{
	m_status = fileStatus;
}

PricePairPtr Cdb_PricePair_Real::GetPricePair(StrategyIdHashId strategyIdHashId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto pos = m_pricePairs.find(strategyIdHashId);
	if (pos != m_pricePairs.end()) return pos->second;

	//string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	PricePairPtr back = std::make_shared<PricePair>();
	m_pricePairs[strategyIdHashId] = back;
	m_status = FileStatus::Updated;
	return back;

}

void Cdb_PricePair_Real::Clear(StrategyIdHashId strategyIdHashId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//string temstr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
	m_status = FileStatus::Updated;
	for (auto& item : m_pricePairs)
	{
		if (item.first == strategyIdHashId && item.second)
		{
			item.second->day = 0;
			item.second->openPrice = 0.0;
			item.second->stopLossPrice = 0.0;

		}
	}
}

void Cdb_PricePair_Real::GetAll(std::map<StrategyIdHashId, PricePairPtr>& pricePairs)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	pricePairs = m_pricePairs;
	return;

}

void Cdb_PricePair_Real::PrintAll()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (const auto& one : m_pricePairs)
	{
		StrategyParamPtr pParam = Get_StrategyParamEnv()->Get_StrategyParam(one.first);
		Log_Print(LogLevel::Info, fmt::format(
			"{} "
			"{} "
			"openPrice:{} "
			"stopLossPrice:{} \n"
			"{} "
			"{} "
			"param1:{:.2f} "
			"ExtreamePrice:{:.2f}({}) "
			"targetPrice:{:.2f} "
			,
			pParam->key.ToStr().c_str(),
			CGlobal::GetTickTimeStr(one.second->day).substr(4, 4).c_str(),
			one.second->openPrice,
			one.second->stopLossPrice,
			CGlobal::GetTickTimeStr(one.second->freshPart.freshTime).c_str(),
			CTransToStr::Get_TrendType(one.second->freshPart.trendType).c_str(),
			one.second->freshPart.param1,
			one.second->freshPart.sideExtreamePrice,
			CGlobal::GetTickTimeStr(one.second->freshPart.sideExtreamePriceTime).c_str(),
			one.second->freshPart.targetPrice
		));

	}
}


