#include "pch.h"
#include "PrintMsgOnce_Real.h"

#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace spdlog;
using namespace sinks;

CPrintMsgOnce_Real::CPrintMsgOnce_Real()
{
	auto dup_filter = std::make_shared<dup_filter_sink_mt>(std::chrono::seconds(5));
	dup_filter->add_sink(std::make_shared<stdout_color_sink_mt>());

	m_pLog = std::make_shared<spdlog::logger>("logger", dup_filter);
	m_pLog->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

}

CPrintMsgOnce_Real::~CPrintMsgOnce_Real()
{

}

void CPrintMsgOnce_Real::PrintOnce(LogLevel level, const std::string& msg)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	switch (level)
	{
	case LogLevel::Info:
		m_pLog->info("{}", msg.c_str());
		break;
	case LogLevel::Warn:
		m_pLog->warn("{}", msg.c_str());
		break;
	case LogLevel::Err:
		m_pLog->error("{}", msg.c_str());
		break;
	case LogLevel::Critical:
		m_pLog->critical("{}", msg.c_str());
		break;
	default:
		break;
	}
	return;

}
