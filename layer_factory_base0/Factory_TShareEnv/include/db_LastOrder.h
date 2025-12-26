#pragma once
#include <base_trade.h>
// 模块最后一次发单时间，用于避免相同BAR重复委托 时间是M1时间标
class Cdb_LastOrder
{
public:
	Cdb_LastOrder() { ; };
	virtual ~Cdb_LastOrder() { ; };

	virtual FileStatus				GetStatus() = 0;

	virtual void					SetStatus(FileStatus fileStatus) = 0;


	virtual void					UpdateLastOrderNo(const OrderKey& orderkey, LocalOrderNo orderNo) = 0;

	virtual void					UpdateLastOrderDealTime(const OrderKey& orderkey, Tick_T ms) = 0;

	virtual Tick_T					GetLastOrderDealTime(const OrderKey& orderkey) = 0;

	virtual LocalOrderNo			GetLastOrderNo(const OrderKey& orderkey) = 0;

	virtual void					PrintAll() = 0;

	virtual size_t					GetCoverCount(StrategyIdHashId id) = 0;

	virtual void					UpdateCoverCount(StrategyIdHashId id, size_t count) = 0;

	virtual void					Clear(StrategyIdHashId id) = 0;

	virtual void					GetAll(std::map<OrderKey, Tick_T>& lastOrderDealTimes, std::map<OrderKey, LocalOrderNo>& lastOrderNos, std::map<StrategyIdHashId, size_t>& coverCounts) = 0;
};
typedef std::shared_ptr<Cdb_LastOrder> db_LastOrderPtr;
