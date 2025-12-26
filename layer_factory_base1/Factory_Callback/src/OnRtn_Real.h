#pragma once
#include "OnRtn.h"
class COnRtn_Real : public COnRtn
{
public:
	COnRtn_Real() { ; };
	virtual ~COnRtn_Real() { ; };

	virtual void				Execute() = 0;

protected:

	// 删除处于half状态的操作
	void						RemoveActiveAction(StrategyIdHashId strategyId, StSubModule subModule, int localOrderNo);

};

