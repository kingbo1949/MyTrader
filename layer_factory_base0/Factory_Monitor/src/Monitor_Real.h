#pragma once
#include "Monitor.h"

struct MonitorKey
{
	StrategyId		strategyId;
	RoundAction		openOrCover;

	bool operator<(const MonitorKey& right) const
	{
		if (strategyId != right.strategyId)
		{
			return strategyId < right.strategyId;
		}
		return openOrCover < right.openOrCover;
	}


};
class CMonitor_Real : 	public CMonitor
{
public:
	CMonitor_Real() { ; };
	virtual ~CMonitor_Real() { ; };

	// 清除
	virtual void			Clear() override final;

	// 打印最近执行情况
	virtual void			PrintRecentExec() override final;

	// 更新最近执行情况
	virtual void			UpdateRecentExec(const StrategyId& strategyId, RoundAction openOrCover, const std::string& dec) override final;
		
protected:
	std::mutex							m_mutex;
	std::map<MonitorKey, std::string>	m_recentExec;


};

