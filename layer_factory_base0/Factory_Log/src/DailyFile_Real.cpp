#include "pch.h"
#include "DailyFile_Real.h"
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h>
#include "Factory_Log.h"

CDailyFile_Real::CDailyFile_Real(const std::string& logPath)
	:CDailyFile(logPath)
{

}

CDailyFile_Real::~CDailyFile_Real()
{

}

void CDailyFile_Real::Log_AsyncPrintDailyFile(const std::string& id, const std::string& msg, int modSecond, bool flushNow /*= false*/)
{
	bool firstUse = false;
	auto logger = MakeAndGet_Logger(id, firstUse);
	if (!logger)
	{
		exit(-1);
	}
	if (firstUse)
	{
		logger->info("{}", GetFirstLine(id).c_str());
	}

	// 开始输出
	if (modSecond == 1 || time(nullptr) % modSecond == 0)
	{
		logger->info("{}", msg.c_str());
		if (flushNow)
		{
			logger->flush();
		}
	}


}


void CDailyFile_Real::Log_AsyncPrintDailyFile_Flush()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto oneItem : m_mapLoggers)
	{
		oneItem.second->flush();
	}
	return;
}

LoggerPtr CDailyFile_Real::MakeAndGet_Logger(const std::string& loggerName, bool& firstUse)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	try
	{
		std::map<std::string, LoggerPtr>::const_iterator pos = m_mapLoggers.find(loggerName);
		if (pos != m_mapLoggers.end())
		{
			firstUse = false;
			return pos->second;
		}

		firstUse = true;
		std::string fileName = fmt::format("{}{}.log", m_logpath.c_str(), loggerName.c_str());
		LoggerPtr logger = spdlog::create_async<spdlog::sinks::daily_file_sink_mt>(loggerName, fileName, 3, 0);
		logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");

		m_mapLoggers[loggerName] = logger;

		return logger;

	}
	catch (const spdlog::spdlog_ex& ex)
	{
		Log_Print(LogLevel::Critical, ex.what());
		return nullptr;
	}

}

std::string CDailyFile_Real::GetFirstLine(const std::string& loggerName)
{
	std::string back = "";
	if (loggerName == "deal_twocode_stat")
	{
		back = ", strategyId, buyOrSell, module, anchor, unanchor, bidlre, midlre, asklre, pickLre, dealSp, diffSp, missMouth";
	}

	if (loggerName == "send_order_used_time")
	{
		back = ",codeId, strategyId, module, dirver_exec, exec_born, born_beforeSend, beforeSend_afterSend, afterSend_callBack1, callBack1_callBack2, total";
	}

	if (loggerName == "insert_order_used_time")
	{
		back = ",fill, send, total";
	}
	if (loggerName == "mirror_qdata")
	{
		back = ",seqno, hh:mm:ss.millsecond, codeId, vol, turnover, last, bid0->ask0";
	}

	if (loggerName == "check_send_used_time")
	{
		back = ",maxTotalVol, oldVol, flow, cancelOrderCount, openOrderCount, decreaseGernalPsFree, addDb, total";
	}

	if (loggerName == "tickThrough_total")
	{
		back = ", udp_tickTranserRec, "
			"tickTranserRec_transOver, "
			"updateQEnv, "
			"publish_bOrderManagerRec, "
			"manager_bOrderOneRec, "
			"checkBOrder, "

			"notifyApp, "
			"throughTellToInterface, "
			"preCheckInApi, "
			"toRealApi, "
			"realApiSend, "
			"begin_realApi"
			;
	}
	if (loggerName == "tickThrough_half")
	{
		back = ", udp_tickTranserRec, "
			"tickTranserRec_transOver, "
			"updateQEnv, "
			"publish_bOrderManagerRec, "
			"manager_bOrderOneRec, "
			"checkBOrder, "
			;
	}

	return back;
}

