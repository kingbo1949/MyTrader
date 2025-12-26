#pragma once
#include "JSon_LastOrder.h"
class CJSon_LastOrder_Real : public CJSon_LastOrder
{
public:
	CJSon_LastOrder_Real() { ; };
	virtual ~CJSon_LastOrder_Real() { ; };


	virtual void	Save(const std::map<OrderKey, Tick_T>& lastOrderDealTimes, const std::map<OrderKey, LocalOrderNo>& lastOrderNos, const std::map<StrategyIdHashId, size_t>& coverCounts) override final;

	virtual void	Load(std::map<OrderKey, Tick_T>& lastOrderDealTimes, std::map<OrderKey, LocalOrderNo>& lastOrderNos, std::map<StrategyIdHashId, size_t>& coverCounts) override final;

protected:
	std::string					GetFileName();
	Json::Value					MakeValue_LastOrderDealTimes(const std::map<OrderKey, Tick_T>& lastOrderDealTimes);
	void						Make_LastOrderDealTimes(const Json::Value& lastOrderDealTimesValue, std::map<OrderKey, Tick_T>& lastOrderDealTimes);

	Json::Value					MakeValue_LastOrderDealTimes_Item(const OrderKey& orderKey, Tick_T time);
	void						Make_LastOrderDealTimes_Item(const Json::Value& lastOrderDealTimesItemValue, OrderKey& orderKey, Tick_T& time);

	// ------------------------------------------------------------
	Json::Value					MakeValue_LastOrderNos(const std::map<OrderKey, LocalOrderNo>& lastOrderNos);
	void						Make_LastOrderNos(const Json::Value& lastOrderNosValue, std::map<OrderKey, LocalOrderNo>& lastOrderNos);

	Json::Value					MakeValue_LastOrderNos_Item(const OrderKey& orderKey, LocalOrderNo localOrderNo);
	void						Make_LastOrderNos_Item(const Json::Value& lastOrderNosItemValue, OrderKey& orderKey, LocalOrderNo& localOrderNo);

	// ------------------------------------------------------------
	Json::Value					MakeValue_CoverCounts(const std::map<StrategyIdHashId, size_t>& coverCounts);
	void						Make_CoverCounts(const Json::Value& coverCountsValue, std::map<StrategyIdHashId, size_t>& coverCounts);

	Json::Value					MakeValue_CoverCounts_Item(StrategyIdHashId strategyIdHashId, size_t count);
	void						Make_CoverCounts_Item(const Json::Value& coverCountsItemValue, StrategyIdHashId& strategyIdHashId, size_t& count);

};

