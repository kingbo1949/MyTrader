#pragma once

#include <base_trade.h>
#include "CsvTransor.h"
struct StrategyTotalParamEx
{
	StrategyParamPtr				pStrategyParam = std::make_shared<CStrategyParam>();
	int								psVol = 0;
};
typedef std::vector<StrategyTotalParamEx> StrategyTotalParamExs;

class CCsvStrategy : public CCsvTransor
{
public:
	CCsvStrategy();
	virtual ~CCsvStrategy();

	// 从csv文件读取StrategyTotalParamExs
	virtual void				LoadStrategyTotalParams(const std::string& fileName, StrategyTotalParamExs& paramExs) = 0;

	// 保存成json持仓文件
	virtual void				SaveStrategyTotalParamsForPs(const StrategyTotalParamExs& params) = 0;


	// 仅提取StrategyTotalParams,并且canopen和cancover不可以都是false
	StrategyParamPtrs			GetStrategyTotalParams(const StrategyTotalParamExs& paramExs);

	// 提取psVol不为零的记录
	void						GetStrategyTotalParamsForPs(const StrategyTotalParamExs& paramExs, StrategyTotalParamExs& output);




};
typedef std::shared_ptr<CCsvStrategy> CsvStrategyPtr;
