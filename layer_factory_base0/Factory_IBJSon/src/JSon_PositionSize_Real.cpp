#include "pch.h"
#include "JSon_PositionSize_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_HashEnv.h>
#include <Global.h>
void CJSon_PositionSize_Real::Save_PositionSizes(const PositionSizePtrs& positionsizes)
{
	Json::Value positionsziesValue = MakeValue_PositionSizes(positionsizes);
	SaveJSonValue(GetFileName(), positionsziesValue);

}

void CJSon_PositionSize_Real::Load_PositionSizes(PositionSizePtrs& positionsizes)
{
	printf("load %s ... \n", GetFileName().c_str());

	Json::Value positionsziesValue;
	LoadJSonValue(GetFileName(), positionsziesValue);

	positionsizes = Make_PositionSizes(positionsziesValue);

	return;

}

std::string CJSon_PositionSize_Real::GetFileName()
{
	return  "./db/positionsizes.json";
}

Json::Value CJSon_PositionSize_Real::MakeValue_PositionSize(PositionSizePtr positionsize)
{
	Json::Value back;

	back["key"] = Get_StrategyParamEnv()->Get_StrategyParam(positionsize->key)->key.ToStr();

	back["codeHashId"] = std::string(Get_CodeIdEnv()->Get_CodeStrByHashId(positionsize->codeHashId));
	back["longOrShort"] = CTransToStr::Get_LongOrShort(positionsize->longOrShort);
	back["vol"] = positionsize->vol;
	back["avgPrice"] = positionsize->avgPrice;
	back["openTickTime"] = CGlobal::GetTickTimeStr(positionsize->openTickTime);

	return back;

}

PositionSizePtr CJSon_PositionSize_Real::Make_PositionSize(const Json::Value& positionsizeValue)
{
	PositionSizePtr back = std::make_shared<CPositionSize>();

	back->key = Get_StrategyParamEnv()->Get_StrategyParam_Hash(positionsizeValue["key"].asString().c_str());

	back->codeHashId = Get_CodeIdEnv()->Get_CodeId_Hash(positionsizeValue["codeHashId"].asString().c_str());
	back->longOrShort = CTransToStr::Get_LongOrShort(positionsizeValue["longOrShort"].asString());
	back->vol = positionsizeValue["vol"].asDouble();
	back->avgPrice = positionsizeValue["avgPrice"].asDouble();
	back->openTickTime = CGlobal::GetTickTimeByStr(positionsizeValue["openTickTime"].asString());

	//printf("ps time = %s \n", CGlobal::GetTickTimeStr(back->openTickTime).c_str());

	return back;

}


Json::Value CJSon_PositionSize_Real::MakeValue_PositionSizes(const PositionSizePtrs& positonsizes)
{
	Json::Value back;
	for (auto onepositionsize : positonsizes)
	{
		Json::Value item = MakeValue_PositionSize(onepositionsize);

		back.append(item);
	}
	return back;

}

PositionSizePtrs CJSon_PositionSize_Real::Make_PositionSizes(const Json::Value& positonsizesValue)
{
	PositionSizePtrs back;

	for (unsigned int i = 0; i < positonsizesValue.size(); ++i)
	{
		Json::Value item = positonsizesValue[i];
		PositionSizePtr onepositionsize = Make_PositionSize(item);
		back.push_back(onepositionsize);

	}
	return back;

}

