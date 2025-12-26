#include "pch.h"
#include "QCallbackObject_RealTime.h"
#include "Factory_Setup.h"
void CQCallbackObject_RealTime::OnTick(IBTickPtr tick)
{
	//printf("update trade, tick size %d \n", tick->bidAsks.size());
	// 更新行情环境
	UpdateQShareEnv(tick);

	// 驱动策略组
	DriveStategys(tick, RoundAction::Open_Round);
	DriveStategys(tick, RoundAction::Cover_Round);

	// 清理交易回报
	CleanTradeRtnQueue();

	// 驱动守护函数
	Get_DaemonByTick()->Execute();

	return;

}
