#pragma once
#include "OnRtnQueue.h"
#include <concurrentqueue/concurrentqueue.h>
using namespace moodycamel;
class COnRtnQueue_Real : public COnRtnQueue
{
public:
	COnRtnQueue_Real() { ; };
	virtual ~COnRtnQueue_Real() { ; };

	virtual void				AddOne(OnRtnPtr pOne) override final;

	virtual OnRtnPtr			PopOne()  override final;

protected:
	ConcurrentQueue<OnRtnPtr>	m_onRtnQueue;

};



