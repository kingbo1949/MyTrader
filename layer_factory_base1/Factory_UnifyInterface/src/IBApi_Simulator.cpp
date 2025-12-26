#include "pch.h"
#include "IBApi_Simulator.h"
#include <Factory_Simulator.h>
#include <Factory_TShareEnv.h>

IBTickPtrs CIBApi_Simulator::QueryTicks(const CodeStr& codeId, time_t daySecond, MarketType marketType)
{
	IBTickPtrs back;
	return back;
}

IBTickPtrs CIBApi_Simulator::QueryTicks(const CodeStr& codeId, time_t beginSecond, int count)
{
	IBTickPtrs back;
	return back;

}

IBKLinePtrs CIBApi_Simulator::QueryKLine(const CodeStr& codeId, Time_Type timetype, time_t endSecond, int days, bool onlyRegularTime, UseType useType)
{
	IBKLinePtrs back;
	return back;
}

IBKLinePtrs CIBApi_Simulator::QueryKLineRange(const CodeStr& codeId, Time_Type timetype, time_t beginSecond, time_t endSecond, int daysForMinute, bool onlyRegularTime, UseType useType)
{
	IBKLinePtrs back;
	return back;

}

bool CIBApi_Simulator::SendOrder(OneOrderPtr porder)
{
	if (!CIBApi_Real::SendOrder(porder)) return false;

	// 发送到柜台
	MakeAndGet_Simulator()->RecOneOrder(porder);

	return true;

}

bool CIBApi_Simulator::CancelOrder(OneOrderPtr porder, const std::string& manualOrderCancelTime)
{
	if (!CIBApi_Real::CancelOrder(porder, manualOrderCancelTime)) return false;

	// 发送到柜台
	MakeAndGet_Simulator()->RecOneCancel(porder->localOrderNo);

	return true;

}
