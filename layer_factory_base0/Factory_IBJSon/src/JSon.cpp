#include "pch.h"
#include "JSon.h"
#include <fstream>
#include <sstream>
#include <iostream>

#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>


CJSon::CJSon()
{

}

CJSon::~CJSon()
{

}

void CJSon::SaveJSonValue(const std::string& fileName, const Json::Value& jsonValue)
{
	//auto str = jsonValue.toStyledString();
	//std::ofstream ofss;
	//ofss.open(fileName);
	//ofss << str;
	//ofss.close();


	std::ofstream ofs(fileName);
	Json::StreamWriterBuilder wbuilder;
	//wbuilder["indentation"] = "";
	wbuilder.settings_["precision"] = 6;
	std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());
	// Write to file.
	writer->write(jsonValue, &ofs);
	ofs.close();



}

void CJSon::LoadJSonValue(const std::string& fileName, Json::Value& jsonValue)
{

	std::ifstream ifs;
	ifs.open(fileName);
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	ifs.close();

	auto str = buffer.str();


	Json::Reader reader;
	reader.parse(str, jsonValue);

	return;

}

Json::Value CJSon::MakeValue_TradeKey(const TradeKey& tradekey)
{
	Json::Value back;
	back["strategyIdHashId"] = Get_StrategyParamEnv()->Get_StrategyParam(tradekey.strategyIdHashId)->key.ToStr();
	back["stSubModule"] = CTransToStr::Get_StSubModule(tradekey.stSubModule);
	return back;
}

TradeKey CJSon::Make_TradeKey(const Json::Value& tradekeyValue)
{
	StrategyIdHashId strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(tradekeyValue["strategyIdHashId"].asString());
	StSubModule stSubModule = CTransToStr::Get_StSubModule(tradekeyValue["stSubModule"].asString());

	TradeKey back(strategyIdHashId, stSubModule);
	return back;
}
