#include "pch.h"
#include "QCallbackObject_Trade.h"
#include <Factory_QShareEnv.h>
#include <Factory_Strategys.h>
#include <Factory_Callback.h>
#include <Factory_Simulator.h>
void CQCallbackObject_Trade::UpdateQShareEnv(IBTickPtr tick)
{
	MakeAndGet_QEnvManager()->GetEnvOneCode(tick->codeHash)->UpdateOneTick(tick);
	return;
}

void CQCallbackObject_Trade::DriveStategys(IBTickPtr tick, RoundAction openOrCover)
{
	On_QuoteChg_Strategy(*tick, openOrCover);
	return;
}

void CQCallbackObject_Trade::CleanTradeRtnQueue()
{
	OnRtnPtr pOnRtn = nullptr;
	while ((pOnRtn = MakeAndGet_OnRtns()->PopOne()))
	{
		pOnRtn->Execute();
	}
	
}

void CQCallbackObject_Trade::DriveMeetDeal()
{
	MakeAndGet_Simulator()->MeetDeal();
	return;
}
