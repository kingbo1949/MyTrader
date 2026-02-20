#include "pch.h"
#include <Factory_Log.h>
#include <Global.h>
#include "Factory.h"
#include "TimerTask_UpdateIndex.h"
#include "Factory.h"
#include "Calculator/Calculator.h"
#include "../cmd/Cmd_UpdateAllIndexFromTimePos.h"
CTimerTask_UpdateIndex::CTimerTask_UpdateIndex()
{
}
void CTimerTask_UpdateIndex::runTimerTask()
{
	// 每 6 次（约 60 秒）输出一次 RocksDB 运行状态
	if (++m_statsCounter >= 6) {
		m_statsCounter = 0;
		std::string stats = MakeAndGet_Env()->GetEnv().DumpStats();
		Log_AsyncPrintDailyFile("rocksdb_stats", stats, 1, false);
	}

	// 线程池必须全部空闲
	if (!MakeAndGet_MyThreadPool()->isAllIdle()) return ;

	std::set<IQKey> keys = GetNeedUpdate();
	UpdateIndex(keys);

	return;
}

void CTimerTask_UpdateIndex::UpdateIndex(const std::set<IQKey>& keys)
{
	if (keys.empty()) return ;

	time_t beginPos = benchmark_milliseconds();
	for (const auto& key : keys)
	{
		// 查询K线
		IQuery query;
		query.byReqType = 0;
		query.dwSubscribeNum = 2;
		IKLines klines;
		MakeAndGet_Env()->GetDB_KLine()->GetKLines(key.codeId, key.timeType, query, klines);
		if (klines.empty()) continue;

		MakeAndGet_MyThreadPool()->commit(0, CCmd_UpdateAllIndexFromTimePos(key.codeId, key.timeType, klines[0].time));
	}

	time_t endPos = benchmark_milliseconds();
	// std::string str = fmt::format("needUpdates size = {}, wait in TimerTask for {}", keys.size(), endPos - beginPos);
	// Log_AsyncPrintDailyFile("updateindex", str, 1, false);
}



void CTimerTask_UpdateIndex::AddNeedUpdate(const std::string& codeId, ITimeType timetype)
{
	// 期货随时可以投递到线程池, 股票需要按照timeTypeMap的时间间隔投递，避免频繁投递
	std::map<ITimeType, Tick_T> timeTypeMap =
	{
		{ITimeType::M1, 20 * 1000},
		{ITimeType::M5, 60 * 1000},
		{ITimeType::M15, 120 * 1000},
		{ITimeType::M30, 180 * 1000},
		{ITimeType::H1, 300 * 1000},
		{ITimeType::D1, 300 * 1000}
	};
	IQKey key;
	key.codeId = codeId;
	key.timeType = timetype;

	std::lock_guard<std::mutex> lock(m_mutex);

	if (GetSecurityType(codeId) == SecurityType::STK)
	{
		auto pos = m_lastPostTime.find(key);
		if (pos != m_lastPostTime.end())
		{
			Tick_T lastpostTime = pos->second;
			if (benchmark_milliseconds() - lastpostTime < timeTypeMap[timetype]) return ;
		}
		m_lastPostTime[key] = benchmark_milliseconds();
	}
	m_needUpdate.insert(key);

	return;
}


std::set<IQKey> CTimerTask_UpdateIndex::GetNeedUpdate()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::set<IQKey> back;
	back.swap(m_needUpdate);

	return back;
}

