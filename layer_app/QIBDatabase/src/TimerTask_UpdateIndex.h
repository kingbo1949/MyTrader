#pragma once
#include <IceUtil/Timer.h>
#include <QStruc.h>
using namespace IBTrader;

typedef time_t Tick_T;			// 毫秒时间类型

enum class NowOrWait
{
	Now,
	Wait
};


class CTimerTask_UpdateIndex : public IceUtil::TimerTask
{
public:
	CTimerTask_UpdateIndex() ;
	virtual ~CTimerTask_UpdateIndex(void) { ; };

	virtual void					runTimerTask();

	void							AddNeedUpdate(const std::string& codeId, ITimeType timetype, NowOrWait nowOrWait);
	

protected:
	std::mutex						m_mutex;
	Tick_T							m_lastUpdateWait;			// 上次更新指数的时间(可等待)
	std::set<IQKey>					m_needUpdate_wait;			// 需要计算指数的key，可以稍后计算
	std::set<IQKey>					m_needUpdate_now;			// 需要马上计算指数的key

	std::set<IQKey>					GetNeedUpdate(NowOrWait nowOrWait);
	void							UpdateIndex(const std::set<IQKey>& keys);
	void							UpdateIndexWait();
	void							UpdateIndexNow();

};

typedef std::shared_ptr<CTimerTask_UpdateIndex> TimerTask_UpdateIndexPtr;