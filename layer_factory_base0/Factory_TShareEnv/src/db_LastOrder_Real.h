#pragma once

// 最近成交的委托 记录了成交时间和委托号码
#include "db_LastOrder.h"
class Cdb_LastOrder_Real : public Cdb_LastOrder
{
public:
	Cdb_LastOrder_Real() ;
	virtual ~Cdb_LastOrder_Real() { ; };

	virtual FileStatus					GetStatus() override final ;

	virtual void						SetStatus(FileStatus fileStatus)  override final ;


	virtual void						UpdateLastOrderNo(const OrderKey& orderkey, LocalOrderNo orderNo) override final;

	virtual void						UpdateLastOrderDealTime(const OrderKey& orderkey, Tick_T ms) override final;

	virtual Tick_T						GetLastOrderDealTime(const OrderKey& orderkey)  override final;

	virtual LocalOrderNo				GetLastOrderNo(const OrderKey& orderkey) override final;

	virtual void						PrintAll() override final;

	virtual size_t						GetCoverCount(StrategyIdHashId id) override final;

	virtual void						UpdateCoverCount(StrategyIdHashId id, size_t count) override final;

	virtual void						Clear(StrategyIdHashId id) override final;

	virtual void						GetAll(std::map<OrderKey, Tick_T>& lastOrderDealTimes, std::map<OrderKey, LocalOrderNo>& lastOrderNos, std::map<StrategyIdHashId, size_t>& coverCounts) override final;


protected:
	std::mutex								m_mutex;
	std::map<OrderKey, Tick_T>				m_lastOrderDealTimes;
	std::map<OrderKey, LocalOrderNo>		m_lastOrderNos;
	std::map<StrategyIdHashId, size_t>		m_coverCount;			// 当日平仓次数，也是回合次数

	FileStatus								m_status;


};

