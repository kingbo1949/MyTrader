#include "pch.h"
#include "OnRtn_Real.h"

#include "pch.h"
#include "OnRtn_Real.h"
#include <Factory_TShareEnv.h>
void COnRtn_Real::RemoveActiveAction(StrategyIdHashId strategyId, StSubModule subModule, int localOrderNo)
{
	ActionKey key;
	key.strategyIdHashId = strategyId;
	key.stSubModule = subModule;

	MakeAndGet_DbActiveAction()->RemoveOne(key, localOrderNo);
	return;
}
