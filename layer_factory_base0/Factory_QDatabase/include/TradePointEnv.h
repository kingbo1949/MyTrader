#pragma once
#include <base_trade.h>


class CTradePointEnv
{
public:
	CTradePointEnv() { ; };
	virtual ~CTradePointEnv() { ; };

	// 根据品种的交易所，得到指定合约的CTradePoint结构
	virtual CTradePoint		GetTrandPoint(CodeHashId codeId) = 0;

	// 判断并设置状态，应用程序应该用一个线程反复调用它
	virtual void			CheckAndSetStatus() = 0;

	// 得到当前时间的时点描述
	virtual TradePointDec	GetTradePointDecResult(CodeHashId codeId) = 0;

	// 当前是否处于收盘前札平时段
	virtual bool			NearCloseForForceCover(CodeHashId codeId) = 0;

	// 当前是否处于收盘前状态
	virtual bool			NearClose(CodeHashId codeId) = 0;

	// 得到某一个时间的时点描述
	virtual TradePointDec	GetTradePointDec(CodeHashId codeId, time_t millisecondIn) const = 0;


};
typedef std::shared_ptr<CTradePointEnv> TradePointEnvPtr;

