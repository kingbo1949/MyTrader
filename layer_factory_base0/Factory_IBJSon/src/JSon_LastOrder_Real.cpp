#include "pch.h"
#include "JSon_LastOrder_Real.h"
#include <Factory_HashEnv.h>
#include <Global.h>

void CJSon_LastOrder_Real::Save(const std::map<OrderKey, Tick_T>& lastOrderDealTimes, const std::map<OrderKey, LocalOrderNo>& lastOrderNos, const std::map<StrategyIdHashId, size_t>& coverCounts)
{
	Json::Value allValue;

	allValue["lastOrderDealTimes"] = MakeValue_LastOrderDealTimes(lastOrderDealTimes);
	allValue["lastOrderNos"] = MakeValue_LastOrderNos(lastOrderNos);
	allValue["coverCounts"] = MakeValue_CoverCounts(coverCounts);

	SaveJSonValue(GetFileName(), allValue);

}

void CJSon_LastOrder_Real::Load(std::map<OrderKey, Tick_T>& lastOrderDealTimes, std::map<OrderKey, LocalOrderNo>& lastOrderNos, std::map<StrategyIdHashId, size_t>& coverCounts)
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value allValue;
	LoadJSonValue(GetFileName(), allValue);

	Make_LastOrderDealTimes(allValue["lastOrderDealTimes"], lastOrderDealTimes);
	Make_LastOrderNos(allValue["lastOrderNos"], lastOrderNos);
	Make_CoverCounts(allValue["coverCounts"], coverCounts);

	return;

}

std::string CJSon_LastOrder_Real::GetFileName()
{
	return  "./db/lastOrder.json";
}

Json::Value CJSon_LastOrder_Real::MakeValue_LastOrderDealTimes(const std::map<OrderKey, Tick_T>& lastOrderDealTimes)
{
	Json::Value back;
	for (const auto& oneItem : lastOrderDealTimes)
	{
		Json::Value item = MakeValue_LastOrderDealTimes_Item(oneItem.first, oneItem.second);

		back.append(item);
	}
	return back;
}

void CJSon_LastOrder_Real::Make_LastOrderDealTimes(const Json::Value& lastOrderDealTimesValue, std::map<OrderKey, Tick_T>& lastOrderDealTimes)
{
	for (unsigned int i = 0; i < lastOrderDealTimesValue.size(); ++i)
	{
		Json::Value item = lastOrderDealTimesValue[i];
		OrderKey orderKey;
		time_t time = 0;
		Make_LastOrderDealTimes_Item(item, orderKey, time);
		lastOrderDealTimes[orderKey] = time;
	}
	return ;

}

Json::Value CJSon_LastOrder_Real::MakeValue_LastOrderDealTimes_Item(const OrderKey& orderKey, Tick_T time)
{
	Json::Value back;

	back["orderKey"] = MakeValue_TradeKey(orderKey);
	back["time"] = Json::Int64(time);
	back["timestr"] = CGlobal::GetTickTimeStr(time);		// 仅用于显示文件中的时间
	return back;
}

void CJSon_LastOrder_Real::Make_LastOrderDealTimes_Item(const Json::Value& lastOrderDealTimesItemValue, OrderKey& orderKey, Tick_T& time)
{
	orderKey = Make_TradeKey(lastOrderDealTimesItemValue["orderKey"]);
	time = lastOrderDealTimesItemValue["time"].asInt64();
	return;

}

Json::Value CJSon_LastOrder_Real::MakeValue_LastOrderNos(const std::map<OrderKey, LocalOrderNo>& lastOrderNos)
{
	Json::Value back;
	for (const auto& oneItem : lastOrderNos)
	{
		Json::Value item = MakeValue_LastOrderNos_Item(oneItem.first, oneItem.second);

		back.append(item);
	}
	return back;
}

void CJSon_LastOrder_Real::Make_LastOrderNos(const Json::Value& lastOrderNosValue, std::map<OrderKey, LocalOrderNo>& lastOrderNos)
{
	for (unsigned int i = 0; i < lastOrderNosValue.size(); ++i)
	{
		Json::Value item = lastOrderNosValue[i];
		OrderKey orderKey;
		LocalOrderNo localOrderNo = 0;
		Make_LastOrderNos_Item(item, orderKey, localOrderNo);
		lastOrderNos[orderKey] = localOrderNo;
	}
	return;

}

Json::Value CJSon_LastOrder_Real::MakeValue_LastOrderNos_Item(const OrderKey& orderKey, LocalOrderNo localOrderNo)
{
	Json::Value back;

	back["orderKey"] = MakeValue_TradeKey(orderKey);
	back["localOrderNo"] = localOrderNo;
	return back;
}

void CJSon_LastOrder_Real::Make_LastOrderNos_Item(const Json::Value& lastOrderNosItemValue, OrderKey& orderKey, LocalOrderNo& localOrderNo)
{
	orderKey = Make_TradeKey(lastOrderNosItemValue["orderKey"]);
	localOrderNo = lastOrderNosItemValue["localOrderNo"].asInt();
	return;
}

Json::Value CJSon_LastOrder_Real::MakeValue_CoverCounts(const std::map<StrategyIdHashId, size_t>& coverCounts)
{
	Json::Value back;
	for (const auto& oneItem : coverCounts)
	{
		Json::Value item = MakeValue_CoverCounts_Item(oneItem.first, oneItem.second);

		back.append(item);
	}
	return back;
}

void CJSon_LastOrder_Real::Make_CoverCounts(const Json::Value& coverCountsValue, std::map<StrategyIdHashId, size_t>& coverCounts)
{
	for (unsigned int i = 0; i < coverCountsValue.size(); ++i)
	{
		Json::Value item = coverCountsValue[i];
		StrategyIdHashId strategyIdHashId;
		size_t count = 0;
		Make_CoverCounts_Item(item, strategyIdHashId, count);
		coverCounts[strategyIdHashId] = count;
	}
	return;

}

Json::Value CJSon_LastOrder_Real::MakeValue_CoverCounts_Item(StrategyIdHashId strategyIdHashId, size_t count)
{
	Json::Value back;
	back["strategyIdHashId"] = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId)->key.ToStr();
	back["count"] = Json::UInt64(count);
	return back;
}

void CJSon_LastOrder_Real::Make_CoverCounts_Item(const Json::Value& coverCountsItemValue, StrategyIdHashId& strategyIdHashId, size_t& count)
{
	strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(coverCountsItemValue["strategyIdHashId"].asString());
	count = coverCountsItemValue["count"].asInt();
	return ;

}
