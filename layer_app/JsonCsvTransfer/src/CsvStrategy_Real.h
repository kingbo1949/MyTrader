#pragma once
#include "CsvStrategy.h"
class CCsvStrategy_Real : public CCsvStrategy
{
public:
	CCsvStrategy_Real();
	virtual ~CCsvStrategy_Real();

	// 从csv文件读取
	virtual void				LoadStrategyTotalParams(const std::string& fileName, StrategyTotalParamExs& paramExs);

	// 保存成json持仓文件
	virtual void				SaveStrategyTotalParamsForPs(const StrategyTotalParamExs& params);


protected:
	// csv字符串转换成结构
	bool						MakeStrategyTotalParam(const std::string& csvStr, StrategyTotalParamEx& param);

	void 						TransToPss(const StrategyTotalParamExs& params, PositionSizePtrs& pss);

	// 转换成持仓
	PositionSizePtr 			TransToPs(const StrategyTotalParamEx& param);

};

