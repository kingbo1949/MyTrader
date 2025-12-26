#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include "MyLogDefine.h"
using namespace spdlog;

class CPrintMsgOnce
{
public:
	CPrintMsgOnce();
	virtual ~CPrintMsgOnce();

	virtual void		PrintOnce(LogLevel level, const std::string& msg) = 0;


};
typedef std::shared_ptr<CPrintMsgOnce> PrintMsgOncePtr;

