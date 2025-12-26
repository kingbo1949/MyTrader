#include "CsvStrategy_Real.h"
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <fstream>
#include <Factory_IBJSon.h>
#include <spdlog/spdlog.h>
#include <Global.h>
#include <Factory_Log.h>
#include <iostream>
//#include <Factory_QDatabase.h>


#include <Log.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;


CCsvStrategy_Real::CCsvStrategy_Real()
{
}


CCsvStrategy_Real::~CCsvStrategy_Real()
{
}


void CCsvStrategy_Real::LoadStrategyTotalParams(const std::string& fileName, StrategyTotalParamExs& paramExs)
{
	printf("%s \n", fileName.c_str());

	MakeTitleIndex(fileName);


	std::string str = CLog::Instance()->ReadFileToString(fileName);
	std::vector<std::string> strs;
	split(strs, str, is_any_of("\n"), token_compress_off);
	for (const auto& oneLine : strs)
	{
		std::string lineStr = oneLine;
		trim(lineStr);
		if (lineStr == "") continue;
		if (IsTitleLine(lineStr, "codeId")) continue;

		StrategyTotalParamEx oneSample;
		if (!MakeStrategyTotalParam(lineStr, oneSample)) continue;

		paramExs.push_back(oneSample);
	}

}




bool CCsvStrategy_Real::MakeStrategyTotalParam(const std::string& csvStr, StrategyTotalParamEx& param)
{
	std::vector<std::string> strs;
	split(strs, csvStr, is_any_of(","), token_compress_off);
	strs = CGlobal::Trim(strs);

	if (strs.size() <= GetIndex("probeVol") )
	{
		printf("cant split line:\n%s\n", csvStr.c_str());
		exit(-1);
	}
	if (strs[0] == "")
	{
		printf("drop line:[%s] \n", csvStr.c_str());
		return false;
	}

	param.pStrategyParam->canOpen = CTransToStr::Get_Bool(strs[GetIndex("canOpen")]);
	param.pStrategyParam->canCover = CTransToStr::Get_Bool(strs[GetIndex("canCover")]);;
	if (!param.pStrategyParam->canOpen && !param.pStrategyParam->canCover)
	{
		return false;
	}
	param.pStrategyParam->timetype = CTransToStr::Get_TimeType(strs[GetIndex("timetype")]);
	param.pStrategyParam->useInterval = CTransToStr::Get_Bool(strs[GetIndex("useInterval")]);;
	param.pStrategyParam->useTimeInterval = CTransToStr::Get_Bool(strs[GetIndex("useTimeInterval")]);;
	param.pStrategyParam->canChgBaseCount = atoi(strs[GetIndex("canChgBaseCount")].c_str());
	param.pStrategyParam->maxTradeCount = atoi(strs[GetIndex("maxTradeCount")].c_str());


	param.pStrategyParam->key.strategyName = strs[GetIndex("strategyName")];
	param.pStrategyParam->key.targetCodeId = strs[GetIndex("codeId")];

	if (strs[GetIndex("buyOrSell")] == CTransToStr::Get_BuyOrSell(BuyOrSell::Buy))
	{
		param.pStrategyParam->key.buyOrSell = BuyOrSell::Buy;
	}
	else
	{
		param.pStrategyParam->key.buyOrSell = BuyOrSell::Sell;
	}
	param.pStrategyParam->key.id = atoi(strs[GetIndex("id")].c_str());


	param.pStrategyParam->proTrendParam1 = atof(strs[GetIndex("proTrendParam1")].c_str());
	param.pStrategyParam->antiTrendParam1 = atof(strs[GetIndex("antiTrendParam1")].c_str());

	param.pStrategyParam->param2 = atof(strs[GetIndex("param2")].c_str());
	param.pStrategyParam->line1 = atof(strs[GetIndex("line1")].c_str());
	param.pStrategyParam->line2 = atof(strs[GetIndex("line2")].c_str());


	param.pStrategyParam->maxVol = atof(strs[GetIndex("maxVol")].c_str());
	param.pStrategyParam->probeVol = atof(strs[GetIndex("probeVol")].c_str());

	if (strs.size() > GetIndex("psVol"))
	{
		param.psVol = atoi(strs[GetIndex("psVol")].c_str());
	}

	return true;

}


void CCsvStrategy_Real::SaveStrategyTotalParamsForPs(const StrategyTotalParamExs& params)
{
	PositionSizePtrs pss;

	TransToPss(params, pss);
	MakeAndGet_JSonPositionSize()->Save_PositionSizes(pss);
	return;

}

void  CCsvStrategy_Real::TransToPss(const StrategyTotalParamExs& params, PositionSizePtrs& pss)
{
	for (auto oneParam : params)
	{
		PositionSizePtr ps = TransToPs(oneParam);
		if (ps)
		{
			pss.push_back(ps);
		}
	}

	return ;

}

PositionSizePtr  CCsvStrategy_Real::TransToPs(const StrategyTotalParamEx& param)
{
	PositionSizePtr positionsize = std::make_shared<CPositionSize>();

	Log_Print(LogLevel::Info, fmt::format("Strategy range[{},{}], key = {}",
		Get_StrategyParamEnv()->Get_StrategyId_Range().beginPos, 
		Get_StrategyParamEnv()->Get_StrategyId_Range().endPos,
		param.pStrategyParam->key.ToStr()
		
		));

	PositonSizeKey mainKey = Get_StrategyParamEnv()->Get_StrategyParam_Hash(param.pStrategyParam->key.ToStr());
	positionsize->key = mainKey;
	positionsize->codeHashId = Get_CodeIdEnv()->Get_CodeId_Hash(param.pStrategyParam->key.targetCodeId.c_str());
	if (param.pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		positionsize->longOrShort = LongOrShort::LongT;
	}
	else
	{
		positionsize->longOrShort = LongOrShort::ShortT;
	}
	positionsize->vol = param.psVol;
	positionsize->openTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();
	return positionsize;
}



