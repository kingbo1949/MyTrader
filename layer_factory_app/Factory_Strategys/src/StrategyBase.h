#pragma once

#include <CompileDefine.h.h>
#include <base_trade.h>
class CStrategyBase
{
public:
	CStrategyBase() { ; };
	virtual ~CStrategyBase() { ; };

	virtual void						Init() = 0;

	// 设置初始参数
	virtual void						SetTradeParams() = 0;


	// 行情触发
	virtual void						OnRtnTick(CodeHashId codeHash, RoundAction openOrCover) = 0;

	///交易回报
	virtual void						OnRtnTrade(StrategyIdHashId strategyIdHashId) = 0;


};

typedef std::shared_ptr<CStrategyBase> StrategyBasePtr;
typedef std::vector<StrategyBasePtr> StrategyBasePtrs;