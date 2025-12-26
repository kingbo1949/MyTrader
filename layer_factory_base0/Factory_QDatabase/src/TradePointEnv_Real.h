#pragma once
#include "TradePointEnv.h"
class CTradePointEnv_Real : public CTradePointEnv
{
public:
	CTradePointEnv_Real() ;
	virtual ~CTradePointEnv_Real() { ; };

	// 根据品种的交易所，得到指定合约的CTradePoint结构
	virtual CTradePoint		GetTrandPoint(CodeHashId codeId) override final;

	// 判断并设置状态，应用程序应该用一个线程反复调用它
	virtual void			CheckAndSetStatus() override final;

	// 得到当前时间的时点描述
	virtual TradePointDec	GetTradePointDecResult(CodeHashId codeId) override final;

	// 当前是否处于收盘前札平时段
	virtual bool			NearCloseForForceCover(CodeHashId codeId)  override final;

	// 当前是否处于收盘前状态
	virtual bool			NearClose(CodeHashId codeId)  override final;

	// 得到某一个时间的时点描述
	virtual TradePointDec	GetTradePointDec(CodeHashId codeId, time_t millisecondIn) const  override final;


protected:
	// 品种与时点的定义关系
	CTradePoint				m_tradePoints[CodeHashIdSize];

	// 品种与当前时点的描述
	TradePointDec			m_tpDecs[CodeHashIdSize];

	// 输入时间（毫秒）与指定时点，得到针对该时点的描述
	TradePointDec			GetTradePointDec(time_t millisecond, TradePointStatus status, const CTradePoint&  tradePoint) const;

	// 比较两个时点描述，返回更近的时点描述
	TradePointDec			CompareTPDec(const TradePointDec& dec1, const TradePointDec& dec2) const;

	// 根据品种的交易所，得到指定合约的CTradePoint结构
	CTradePoint				GetTrandPoint(CodeHashId codeId, const TradePointPtrs& allTimePoints);







};

