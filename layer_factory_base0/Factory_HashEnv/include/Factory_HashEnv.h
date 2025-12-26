#pragma once

#ifdef WIN32

#ifdef FACTORY_HASHENV_EXPORTS
#define FACTORY_HASHENV_API __declspec(dllexport)
#else
#define FACTORY_HASHENV_API __declspec(dllimport)
#endif


#else

#define FACTORY_HASHENV_API 

#endif // WIN32


#include <CompileDefine.h.h>
#include <base_trade.h>

#include "CodeIdHashEnv.h"
#include "StrategyParamHashEnv.h"
#include "ChoiceHelper.h"

#ifndef WIN32
extern "C"
{
#endif // __cplusplus

	FACTORY_HASHENV_API void					Make_CodeIdEnv(const std::set<CodeStr>& vaildCodeIs);

	FACTORY_HASHENV_API CodeIdHashEnvPtr		Get_CodeIdEnv();

	FACTORY_HASHENV_API void					Make_StrategyParamEnv(const StrategyParamPtrs& params);

	FACTORY_HASHENV_API StrategyParamHashEnvPtr Get_StrategyParamEnv();

	class FACTORY_HASHENV_API  					CChoiceHelper;


	FACTORY_HASHENV_API StrategyIdHashId		My_BKDR_Str_Hash(const char* str);

	//inline StrategyIdHashId	My_BKDR_Str_Hash(const char* str)
	//{
	//	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	//	unsigned int hash = 0;

	//	while (*str)
	//	{
	//		hash = hash * seed + (*str++);
	//	}

	//	return (hash & 0x7FFFFFFF);

	//}

#ifndef WIN32
}
#endif // __cplusplus



