#pragma once
#include <IceUtil/Timer.h>
#include <QStruc.h>
using namespace IBTrader;

typedef time_t Tick_T;

class CTimerTask_UpdateIndex : public IceUtil::TimerTask
{
public:
	CTimerTask_UpdateIndex() ;
	virtual ~CTimerTask_UpdateIndex(void) { ; };

	virtual void					runTimerTask();

	void							AddNeedUpdate(const std::string& codeId, ITimeType timetype);
	

protected:
	std::mutex						m_mutex;
	std::set<IQKey>					m_needUpdate;			// 需要更新的品种
	int								m_statsCounter = 0;		// DumpStats 计数器
	std::map<IQKey, Tick_T>			m_lastPostTime;			// 每种合约上次投递到线程池的时间

	std::set<IQKey>					GetNeedUpdate();
	void							UpdateIndex(const std::set<IQKey>& keys);

};

typedef std::shared_ptr<CTimerTask_UpdateIndex> TimerTask_UpdateIndexPtr;