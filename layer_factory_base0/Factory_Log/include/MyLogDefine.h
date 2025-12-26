#pragma once

#include <spdlog/spdlog.h>
enum class LogLevel
{
	Info = 0,
	Warn = 1,			
	Err = 2,		
	Critical = 3	

};
typedef std::shared_ptr<spdlog::logger> LoggerPtr;


