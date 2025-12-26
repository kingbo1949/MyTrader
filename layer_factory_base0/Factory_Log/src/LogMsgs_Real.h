#pragma once
#include "LogMsgs.h"
#include <vector>
#include <mutex>

class CLogMsgs_Real : public CLogMsgs
{
public:
	CLogMsgs_Real();
	virtual ~CLogMsgs_Real();

	virtual void				AddMsg(const OneLogMsg& onemsg) override final;

	virtual void 				HandleAllMsgs() override final;

protected:
	std::mutex					m_mutex;
	std::vector<OneLogMsg>		m_msgs;


};

