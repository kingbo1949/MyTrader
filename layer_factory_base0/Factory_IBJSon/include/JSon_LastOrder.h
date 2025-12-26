#pragma once
#include "JSon.h"
#include <base_trade.h>
class CJSon_LastOrder : public CJSon
{
public:
	CJSon_LastOrder() { ; };
	virtual ~CJSon_LastOrder() { ; };

	virtual void			Save(const std::map<OrderKey, Tick_T>& lastOrderDealTimes, const std::map<OrderKey, LocalOrderNo>& lastOrderNos, const std::map<StrategyIdHashId, size_t>& coverCounts) = 0;

	virtual void			Load(std::map<OrderKey, Tick_T>& lastOrderDealTimes, std::map<OrderKey, LocalOrderNo>& lastOrderNos, std::map<StrategyIdHashId, size_t>& coverCounts) = 0;
};
typedef std::shared_ptr<CJSon_LastOrder> JSon_LastOrderPtr;



