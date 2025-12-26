#include "pch.h"
#include "JSon_StrategyParam_Real.h"
#include <Factory_IBGlobalShare.h>

void CJSon_StrategyParam_Real::Save_StrategyParams(const StrategyParamPtrs& strategyParams)
{
	Json::Value strategyParamsValue = MakeValue_StrategyParams(strategyParams);
	SaveJSonValue(GetFileName(), strategyParamsValue);
}

void CJSon_StrategyParam_Real::Load_StrategyParams(StrategyParamPtrs& strategyParams)
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value strategyParamsValue;
	LoadJSonValue(GetFileName(), strategyParamsValue);

	strategyParams = Make_StrategyParams(strategyParamsValue);

	return;

}

std::string CJSon_StrategyParam_Real::GetFileName()
{
	return "./db/strategyParams.json";
}

Json::Value CJSon_StrategyParam_Real::MakeValue_Strategykey(const Strategykey& strategykey)
{
	Json::Value back;
	back["strategyName"] = strategykey.strategyName;
	back["targetCodeId"] = strategykey.targetCodeId;
	back["BuyOrSell"] = CTransToStr::Get_BuyOrSell(strategykey.buyOrSell);
	back["id"] = strategykey.id;

	return back;

}

Strategykey CJSon_StrategyParam_Real::Make_Strategykey(const Json::Value& StrategykeyValue)
{
	Strategykey back;

	back.strategyName = StrategykeyValue["strategyName"].asString();
	back.targetCodeId = StrategykeyValue["targetCodeId"].asString();
	back.buyOrSell = CTransToStr::Get_BuyOrSell(StrategykeyValue["BuyOrSell"].asString());
	back.id = StrategykeyValue["id"].asInt();

	return back;

}

Json::Value CJSon_StrategyParam_Real::MakeValue_StrategyParam(const StrategyParamPtr& strategyParam)
{
	Json::Value back;
	back["key"] = MakeValue_Strategykey(strategyParam->key);
	back["canOpen"] = strategyParam->canOpen;
	back["canCover"] = strategyParam->canCover;
	back["timetype"] = CTransToStr::Get_TimeType(strategyParam->timetype);

	back["useInterval"] = strategyParam->useInterval;
	back["useTimeInterval"] = strategyParam->useTimeInterval;
	back["canChgBaseCount"] = strategyParam->canChgBaseCount;
	back["maxTradeCount"] = strategyParam->maxTradeCount;

	back["proTrendParam1"] = strategyParam->proTrendParam1;
	back["antiTrendParam1"] = strategyParam->antiTrendParam1;
	back["param2"] = strategyParam->param2;
	back["line1"] = strategyParam->line1;
	back["line2"] = strategyParam->line2;

	back["probeVol"] = strategyParam->probeVol;
	back["maxVol"] = strategyParam->maxVol;
	return back;

}

StrategyParamPtr CJSon_StrategyParam_Real::Make_StrategyParam(const Json::Value& strategyParamValue)
{
	StrategyParamPtr back = std::make_shared<CStrategyParam>();

	back->key = Make_Strategykey(strategyParamValue["key"]);
	back->canOpen = strategyParamValue["canOpen"].asBool();
	back->canCover = strategyParamValue["canCover"].asBool();
	back->timetype = CTransToStr::Get_TimeType(strategyParamValue["timetype"].asString());

	back->useInterval = strategyParamValue["useInterval"].asBool();
	back->useTimeInterval = strategyParamValue["useTimeInterval"].asBool();
	back->canChgBaseCount = strategyParamValue["canChgBaseCount"].asInt();
	back->maxTradeCount = strategyParamValue["maxTradeCount"].asInt();

	back->proTrendParam1 = strategyParamValue["proTrendParam1"].asDouble();
	back->antiTrendParam1 = strategyParamValue["antiTrendParam1"].asDouble();
	back->param2 = strategyParamValue["param2"].asDouble();
	back->line1 = strategyParamValue["line1"].asDouble();
	back->line2 = strategyParamValue["line2"].asDouble();


	back->probeVol = strategyParamValue["probeVol"].asDouble();
	back->maxVol = strategyParamValue["maxVol"].asDouble();
	return back;
}



Json::Value CJSon_StrategyParam_Real::MakeValue_StrategyParams(const StrategyParamPtrs& strategyParams)
{
	Json::Value back;

	for (StrategyParamPtrs::const_iterator pos = strategyParams.begin(); pos != strategyParams.end(); ++pos)
	{
		Json::Value item = MakeValue_StrategyParam(*pos);
		back.append(item);
	}
	return back;

}

StrategyParamPtrs CJSon_StrategyParam_Real::Make_StrategyParams(const Json::Value& strategyParamsValue)
{
	StrategyParamPtrs back;

	for (unsigned int i = 0; i < strategyParamsValue.size(); ++i)
	{
		Json::Value item = strategyParamsValue[i];
		StrategyParamPtr  pStrategyParam = Make_StrategyParam(item);
		back.push_back(pStrategyParam);
	}
	return back;

}
