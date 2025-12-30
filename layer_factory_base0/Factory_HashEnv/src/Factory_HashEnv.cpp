#include "pch.h"
#include "Factory_HashEnv.h"

#include "CodeIdHashEnv_Real.h"
#include "StrategyParamHashEnv_Real.h"


CodeIdHashEnvPtr g_pCodeIdHashEnv = nullptr;
void Make_CodeIdEnv(const std::set<CodeStr>& vaildCodeIs)
{
	if (!g_pCodeIdHashEnv)
	{
		g_pCodeIdHashEnv = std::make_shared<CCodeIdHashEnv_Real>(vaildCodeIs);
	}
	return;
}


CodeIdHashEnvPtr Get_CodeIdEnv()
{
	if (!g_pCodeIdHashEnv)
	{
		printf("g_pCodeIdHashEnv is null \n");
		exit(-1);
	}
	return g_pCodeIdHashEnv;
}

StrategyParamHashEnvPtr g_pStrategyParamHashEnv = nullptr;

void Make_StrategyParamEnv(const StrategyParamPtrs& params)
{
	if (!g_pStrategyParamHashEnv)
	{
		g_pStrategyParamHashEnv = std::make_shared<CStrategyParamHashEnv_Real>(params);
	}
	return;


}

StrategyParamHashEnvPtr Get_StrategyParamEnv()
{
	if (!g_pStrategyParamHashEnv)
	{
		printf("g_pStrategyParamHashEnv is null \n");
		exit(-1);
	}
	return g_pStrategyParamHashEnv;
}

StrategyIdHashId My_BKDR_Str_Hash(const char* str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

