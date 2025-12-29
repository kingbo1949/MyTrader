#pragma once

#include "Factory_HashEnv_Export.h"


#include <CompileDefine.h.h>
#include <base_trade.h>

#include "CodeIdHashEnv.h"
#include "StrategyParamHashEnv.h"
#include "ChoiceHelper.h"

FACTORY_HASHENV_API void					Make_CodeIdEnv(const std::set<CodeStr>& vaildCodeIs);

FACTORY_HASHENV_API CodeIdHashEnvPtr		Get_CodeIdEnv();

FACTORY_HASHENV_API void					Make_StrategyParamEnv(const StrategyParamPtrs& params);

FACTORY_HASHENV_API StrategyParamHashEnvPtr Get_StrategyParamEnv();

class   					                CChoiceHelper;


FACTORY_HASHENV_API StrategyIdHashId		My_BKDR_Str_Hash(const char* str);




