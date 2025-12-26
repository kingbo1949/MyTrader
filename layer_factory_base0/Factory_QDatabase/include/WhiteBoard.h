#pragma once
#include <base_struc.h>
// 白板行情
class CWhiteBoard
{
public:
	CWhiteBoard() { ; };
	virtual ~CWhiteBoard() { ; };

	virtual void			Init() = 0;

	// 查询tick
	virtual IBTickPtr		GetTick(CodeHashId codeId) = 0;

	// 更新开盘价, 不介意时间戳, 由tickPrice函数返回Field: 14, 不生成tick
	virtual void			UpdateOpen(CodeHashId codeId, double open) = 0;

	// 更新bid价, 不介意时间戳, 由tickPrice函数返回, 不生成tick
	virtual void			UpdateBid(CodeHashId codeId, double bidprice) = 0;

	// 更新ask价, 不介意时间戳, 由tickPrice函数返回, 不生成tick
	virtual void			UpdateAsk(CodeHashId codeId, double askprice) = 0;

	// 更新bidvol, 介意时间戳, 由ticksize函数返回, 生成tick
	virtual IBTickPtr		UpdateBidVol(CodeHashId codeId, int bidVol) = 0;

	// 更新askvol, 介意时间戳, 由ticksize函数返回, 生成tick
	virtual IBTickPtr		UpdateAskVol(CodeHashId codeId, int askVol) = 0;

	// 更新累计成交量, 介意时间戳, 有tickSize返回Field: 8, , 生成tick
	virtual IBTickPtr		UpdateTotalVol(CodeHashId codeId, int totalvol) = 0;

	virtual void			UpdateLastPrice(CodeHashId codeId, double last) = 0;

	virtual IBTickPtr		UpdateLastVol(CodeHashId codeId, int lastVol) = 0;

	// 更新bidask, 介意时间戳, 由tickByTickBidAsk函数返回, 生成tick
	virtual IBTickPtr		UpdateBidAsk(CodeHashId codeId, double bid, int bidvol, double ask, int askvol) = 0;
};
typedef std::shared_ptr<CWhiteBoard> WhiteBoardPtr;


