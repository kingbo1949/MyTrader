#pragma once
#include "JSon_StrategyParam.h"
class CJSon_StrategyParam_Real : public CJSon_StrategyParam
{
public:
	CJSon_StrategyParam_Real() { ; };
	virtual ~CJSon_StrategyParam_Real() { ; };

	virtual void			Save_StrategyParams(const StrategyParamPtrs& strategyParams) override final;

	virtual void			Load_StrategyParams(StrategyParamPtrs& strategyParams) override final;

protected:
	std::string				GetFileName();


	Json::Value				MakeValue_Strategykey(const Strategykey& strategykey);
	Strategykey				Make_Strategykey(const Json::Value& StrategykeyValue);

	Json::Value				MakeValue_StrategyParam(const StrategyParamPtr& strategyParam);
	StrategyParamPtr		Make_StrategyParam(const Json::Value& strategyParamValue);

	Json::Value				MakeValue_StrategyParams(const StrategyParamPtrs& strategyParams);
	StrategyParamPtrs		Make_StrategyParams(const Json::Value& strategyParamsValue);




};

