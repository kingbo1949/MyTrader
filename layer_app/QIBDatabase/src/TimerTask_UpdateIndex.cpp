#include "pch.h"
#include <Factory_Log.h>
#include "TimerTask_UpdateIndex.h"
#include "Factory.h"
#include "Calculator.h"
CTimerTask_UpdateIndex::CTimerTask_UpdateIndex()
{
	m_lastUpdateWait = 0;
}
void CTimerTask_UpdateIndex::runTimerTask()
{
	UpdateIndexNow();
	UpdateIndexWait();

	return;
}
void CTimerTask_UpdateIndex::UpdateIndexWait()
{
	Tick_T currentTime = benchmark_milliseconds();
	if (currentTime - m_lastUpdateWait < 20000) return;		// 每20秒更新一次即可

	std::set<IQKey> needUpdates = GetNeedUpdate(NowOrWait::Wait);
	if (needUpdates.empty()) return;
	UpdateIndex(needUpdates);
	m_lastUpdateWait = benchmark_milliseconds();
	return;
}

void CTimerTask_UpdateIndex::UpdateIndexNow()
{
	std::set<IQKey> needUpdates = GetNeedUpdate(NowOrWait::Now);
	if (needUpdates.empty()) return;

	UpdateIndex(needUpdates);
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

		// 更新指标
		MakenAndGet_Calculator_Ma()->Update(key.codeId, key.timeType, klines.back());
		//MakenAndGet_Calculator_VwMa()->Update(key.codeId, key.timeType, klines.back());
		//MakenAndGet_Calculator_Ema()->Update(key.codeId, key.timeType, klines.back());
		MakenAndGet_Calculator_Macd()->Update(key.codeId, key.timeType, klines.back());
		MakenAndGet_Calculator_DivType()->Update(key.codeId, key.timeType, klines.back());
	}
	time_t endPos = benchmark_milliseconds();
	std::string str = fmt::format("needUpdates size = {},  update ma macd DivType used: {}", int(keys.size()), int(endPos - beginPos));
	Log_AsyncPrintDailyFile("updateindex", str, 1, false);
}



void CTimerTask_UpdateIndex::AddNeedUpdate(const std::string& codeId, ITimeType timetype, NowOrWait nowOrWait)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	IQKey key;
	key.codeId = codeId;
	key.timeType = timetype;
	if (nowOrWait == NowOrWait::Now)
	{
		m_needUpdate_now.insert(key);
		
	}
	else
	{
		m_needUpdate_wait.insert(key);
	}
	
	return;
}


std::set<IQKey> CTimerTask_UpdateIndex::GetNeedUpdate(NowOrWait nowOrWait)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::set<IQKey> back;
	if (nowOrWait == NowOrWait::Now)
	{
		back.swap(m_needUpdate_now);
	}
	else
	{
		back.swap(m_needUpdate_wait);
	}
	
	return back;
}

