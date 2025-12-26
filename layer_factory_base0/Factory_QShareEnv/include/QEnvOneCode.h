#pragma once

#include <base_struc.h>

class CTickEnv
{
public:
	CTickEnv() { ; };
	CTickEnv(const CTickEnv& tickEnv)
	{
		pTick = tickEnv.pTick;
		tickIndex = tickEnv.tickIndex;

	};

	virtual ~CTickEnv() { ; };


	IBTickPtr							pTick = nullptr;
	int									tickIndex = -1;
};
typedef std::shared_ptr<CTickEnv> TickEnvPtr;


class CQEnvOneCode
{
public:
	CQEnvOneCode() { ; };
	virtual ~CQEnvOneCode() { ; };

	// 更新新tick
	virtual TickEnvPtr					UpdateOneTick(IBTickPtr ptick) = 0;


	virtual TickEnvPtr					GetTicEnv() = 0;

	virtual TickEnvPtr					GetLastTicEnv() = 0;


};

typedef std::shared_ptr<CQEnvOneCode> QEnvOneCodePtr;