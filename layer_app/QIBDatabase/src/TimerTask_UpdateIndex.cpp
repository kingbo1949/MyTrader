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
	// 等待线程池空闲
	while (!MakeAndGet_MyThreadPool()->isAllIdle())
	{
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
	std::set<IQKey> keys = GetNeedUpdate();
	UpdateIndex(keys);

	return;
}

void CTimerTask_UpdateIndex::UpdateIndex(const std::set<IQKey>& keys)
{
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

		MakeAndGet_MyThreadPool()->commit(CCmd_UpdateAllIndexFromTimePos(key.codeId, key.timeType, klines[0].time));
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

