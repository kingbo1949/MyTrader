#pragma once
#include "PrintMsgOnce.h"


typedef std::shared_ptr<spdlog::logger> LoggerPtr;


class CPrintMsgOnce_Real : public CPrintMsgOnce
{
public:
	CPrintMsgOnce_Real();
	virtual ~CPrintMsgOnce_Real();

	virtual void			PrintOnce(LogLevel level, const std::string& msg) override final ;

protected:
	std::mutex				m_mutex;
	LoggerPtr				m_pLog;
};


