#pragma once
#include <base_trade.h>

class CMonitor
{
public:
	CMonitor() { ; };
	virtual ~CMonitor() { ; };

	// 清除
	virtual void			Clear() = 0;

	// 打印最近执行情况
	virtual void			PrintRecentExec() = 0;

	// 更新最近执行情况
	virtual void			UpdateRecentExec(const StrategyId& strategyId, RoundAction openOrCover, const std::string& dec) = 0;
};
typedef std::shared_ptr<CMonitor> MonitorPtr;



