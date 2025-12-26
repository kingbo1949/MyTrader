#pragma once

// 回报对象队列
#include "OnRtn.h"
class COnRtnQueue
{
public:
	COnRtnQueue() { ; };
	virtual ~COnRtnQueue() { ; };

	virtual void			AddOne(OnRtnPtr pOne) = 0;

	virtual OnRtnPtr		PopOne() = 0;

};

typedef std::shared_ptr<COnRtnQueue> OnRtnQueuePtr;

