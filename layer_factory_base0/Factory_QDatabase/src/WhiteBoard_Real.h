#pragma once
#include "WhiteBoard.h"

//盘后启动，可以去数据库里取; 取不到，则可以等待open的回推更新
//
//盘前启动，可以等着到开盘后将第一个last转换成open

class CWhiteBoard_Real : public CWhiteBoard
{
public:
	CWhiteBoard_Real();
	virtual ~CWhiteBoard_Real() { ; };

	virtual void			Init() override final;

	// 查询tick
	virtual IBTickPtr		GetTick(CodeHashId codeId) override final;

	// 更新开盘价, 不介意时间戳, 由tickPrice函数返回Field: 14, 不生成tick
	virtual void			UpdateOpen(CodeHashId codeId, double open) override final;

	// 更新bid价, 不介意时间戳, 由tickPrice函数返回, 不生成tick
	virtual void			UpdateBid(CodeHashId codeId, double bidprice) override final;

	// 更新ask价, 不介意时间戳, 由tickPrice函数返回, 不生成tick
	virtual void			UpdateAsk(CodeHashId codeId, double askprice) override final;

	// 更新bidvol, 介意时间戳, 由ticksize函数返回, 生成tick
	virtual IBTickPtr		UpdateBidVol(CodeHashId codeId, int bidVol) override final;

	// 更新askvol, 介意时间戳, 由ticksize函数返回, 生成tick
	virtual IBTickPtr		UpdateAskVol(CodeHashId codeId, int askVol) override final;

	// 更新累计成交量, 介意时间戳, 有tickSize返回Field: 8, , 生成tick
	virtual IBTickPtr		UpdateTotalVol(CodeHashId codeId, int totalvol)  override final;

	virtual void			UpdateLastPrice(CodeHashId codeId, double last) override final;

	virtual IBTickPtr		UpdateLastVol(CodeHashId codeId, int lastVol) override final;

	// 更新bidask, 介意时间戳, 由tickByTickBidAsk函数返回, 生成tick
	virtual IBTickPtr		UpdateBidAsk(CodeHashId codeId, double bid, int bidvol, double ask, int askvol)  override final;

protected:
	IBTickPtr				m_board[CodeHashIdSize];

	void					Init(CodeHashId codeId);

	// 得到某一个时间是否处于开盘状态
	bool					IsInTradeStatus(CodeHashId codeId, time_t millisecondIn);

	bool					ValidTick(IBTickPtr tick);

	// 把tick的bidasks的size重置为1
	void					TrimTick(IBTickPtr tick);


};

