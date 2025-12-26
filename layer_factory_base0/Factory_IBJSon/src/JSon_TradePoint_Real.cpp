#include "pch.h"
#include "JSon_TradePoint_Real.h"
#include <Factory_IBGlobalShare.h>
void CJSon_TradePoint_Real::Save_TradePoints(const TradePointPtrs& tradePoints)
{
	Json::Value tradePointsValue = MakeValue_TradePoints(tradePoints);
	SaveJSonValue(GetFileName(), tradePointsValue);

}

TradePointPtrs CJSon_TradePoint_Real::Load_TradePoints()
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value tradePointsValue;
	LoadJSonValue(GetFileName(), tradePointsValue);

	TradePointPtrs back = Make_TradePoints(tradePointsValue);

	return back;

}

std::string CJSon_TradePoint_Real::GetFileName()
{
	return "./db/tradePoints.json";
}

Json::Value CJSon_TradePoint_Real::MakeValue_OneTradePoint(const TradePointPtr& oneTradePoint)
{
	Json::Value back;
	back["primaryExchangePl"] = CTransToStr::Get_ExchangePl(oneTradePoint->primaryExchangePl);
	back["openMorning"] = oneTradePoint->openMorning;
	back["closeMorning"] = oneTradePoint->closeMorning;

	back["openAfternoon"] = oneTradePoint->openAfternoon;
	back["closeAfternoon"] = oneTradePoint->closeAfternoon;

	back["openEvening"] = oneTradePoint->openEvening;
	back["closeEvening"] = oneTradePoint->closeEvening;

	back["breakBegin"] = oneTradePoint->breakBegin;
	back["breakEnd"] = oneTradePoint->breakEnd;

	return back;

}

TradePointPtr CJSon_TradePoint_Real::Make_OneTradePoint(const Json::Value& oneTradePointValue)
{
	TradePointPtr back = std::make_shared<CTradePoint>();

	back->primaryExchangePl = CTransToStr::Get_ExchangePl(oneTradePointValue["primaryExchangePl"].asString());
	back->openMorning = oneTradePointValue["openMorning"].asString();
	back->closeMorning = oneTradePointValue["closeMorning"].asString();

	back->openAfternoon = oneTradePointValue["openAfternoon"].asString();
	back->closeAfternoon = oneTradePointValue["closeAfternoon"].asString();

	back->openEvening = oneTradePointValue["openEvening"].asString();
	back->closeEvening = oneTradePointValue["closeEvening"].asString();

	back->breakBegin = oneTradePointValue["breakBegin"].asString();
	back->breakEnd = oneTradePointValue["breakEnd"].asString();

	return back;

}

Json::Value CJSon_TradePoint_Real::MakeValue_TradePoints(const TradePointPtrs& tradePoints)
{
	Json::Value back;
	for (TradePointPtrs::const_iterator pos = tradePoints.begin(); pos != tradePoints.end(); ++pos)
	{
		Json::Value item = MakeValue_OneTradePoint(*pos);

		back.append(item);
	}
	return back;

}

TradePointPtrs CJSon_TradePoint_Real::Make_TradePoints(const Json::Value& tradePointsValue)
{
	TradePointPtrs back;

	for (unsigned int i = 0; i < tradePointsValue.size(); ++i)
	{
		Json::Value item = tradePointsValue[i];
		TradePointPtr oneTimePoint = Make_OneTradePoint(item);
		back.push_back(oneTimePoint);

	}
	return back;

}

