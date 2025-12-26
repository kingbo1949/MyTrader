#include "pch.h"
#include "QEnvOneCode_Real.h"

CQEnvOneCode_Real::CQEnvOneCode_Real()
	:m_count(0)
{
	m_env = std::make_shared<CTickEnv>();
	m_lastEnv = std::make_shared<CTickEnv>();
}

TickEnvPtr CQEnvOneCode_Real::UpdateOneTick(IBTickPtr ptick)
{
	// 转移
	m_lastEnv->pTick = m_env->pTick;
	m_lastEnv->tickIndex = m_env->tickIndex;

	// 更新
	++m_count;

	m_env->pTick = ptick;
	m_env->tickIndex = m_count - 1;

	return m_env;

}

TickEnvPtr CQEnvOneCode_Real::GetTicEnv()
{
	return m_env;
}

TickEnvPtr CQEnvOneCode_Real::GetLastTicEnv()
{
	return m_lastEnv;
}
