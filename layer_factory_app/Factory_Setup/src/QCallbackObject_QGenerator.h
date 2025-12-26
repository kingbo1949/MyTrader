#pragma once
#include "QCallbackObject.h"
class CQCallbackObject_QGenerator : public CQCallbackObject
{
public:
	CQCallbackObject_QGenerator();
	virtual ~CQCallbackObject_QGenerator() { ; };

	virtual void					OnTick(IBTickPtr tick) override final;

protected:
	time_t							m_lastUpdateSecond;
	void							UpdateTickToDb(IBTickPtr tick);
	std::map<CodeHashId, IBTickPtr>	m_lastUpdateTicks;

};

