#include "pch.h"
#include <Factory_Log.h>
#include <Global.h>
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
		query.dwSubscribeNum = 1;
		IKLines klines;
		MakeAndGet_Env()->GetDB_KLine()->GetKLines(key.codeId, key.timeType, query, klines);
		if (klines.empty()) continue;

		MakeAndGet_MyThreadPool()->commit(0, CCmd_UpdateAllIndexFromTimePos(key.codeId, key.timeType, klines[0].time));
	}

	time_t endPos = benchmark_milliseconds();
	std::string str = fmt::format("needUpdates size = {}, wait in TimerTask for {}", keys.size(), endPos - beginPos);
	Log_AsyncPrintDailyFile("updateindex", str, 1, false);
}



void CTimerTask_UpdateIndex::AddNeedUpdate(const std::string& codeId, ITimeType timetype)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	IQKey key;
	key.codeId = codeId;
	key.timeType = timetype;
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

