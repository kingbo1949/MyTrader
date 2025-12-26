#include "pch.h"
#include "QCallbackObject_Simulator.h"
#include "Factory_Setup.h"
#include <base_trade.h>
#include <Global.h>
#include <Factory_IBGlobalShare.h>
void CQCallbackObject_Simulator::OnTick(IBTickPtr tick)
{
	// 更新行情环境
	UpdateQShareEnv(tick);

	// 开仓驱动策略组 并撮合
	DriveStategys(tick, RoundAction::Open_Round);
	DriveSimulator();

	// 平仓驱动策略组 并撮合
	DriveStategys(tick, RoundAction::Cover_Round);
	DriveSimulator();

	// 驱动守护函数
	Get_DaemonByTick()->Execute();

	return;

}

void CQCallbackObject_Simulator::DriveSimulator()
{
	// 清理交易回报
	CleanTradeRtnQueue();

	// 撮合模拟成交
	DriveMeetDeal();

	// 再次清理交易回报
	CleanTradeRtnQueue();


}
