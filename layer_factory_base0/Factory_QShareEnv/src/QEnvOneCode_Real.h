#pragma once
#include "QEnvOneCode.h"
class CQEnvOneCode_Real : public CQEnvOneCode
{
public:
	CQEnvOneCode_Real();
	virtual ~CQEnvOneCode_Real() { ; };

	// 更新新tick
	virtual TickEnvPtr					UpdateOneTick(IBTickPtr ptick) override final;


	virtual TickEnvPtr					GetTicEnv() override final;

	virtual TickEnvPtr					GetLastTicEnv() override final;



protected:
	int									m_count;

	TickEnvPtr							m_env;
	TickEnvPtr							m_lastEnv;

};

