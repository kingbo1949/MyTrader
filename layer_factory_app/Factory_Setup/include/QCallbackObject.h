#pragma once

// 行情回调对象 它会被注册到QTopic上，由行情tick驱动它执行
#include <base_struc.h>
class CQCallbackObject
{
public:
	CQCallbackObject() { ; };
	virtual ~CQCallbackObject() { ; };

	virtual void				OnTick(IBTickPtr tick) = 0;

protected:

};
typedef std::shared_ptr<CQCallbackObject> QCallbackObjectPtr;


