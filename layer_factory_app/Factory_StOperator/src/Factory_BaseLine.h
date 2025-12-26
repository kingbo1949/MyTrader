#pragma once

#include "BaseLine.h"
#include "HighLowPointOutside.h"
void					Make_BaseLine(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint);

BaseLinePtr				Get_BaseLine(StrategyIdHashId strategyIdHashId);

HighLowPointOutsidePtr	MakeAndGet_HighLowPointOutside();
