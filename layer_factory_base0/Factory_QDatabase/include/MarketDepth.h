#pragma once
#include <base_trade.h>
class CMarketDepth
{
public:
	CMarketDepth() { ; };
	virtual ~CMarketDepth() { ; };

	virtual void			Init() = 0;

	virtual void			Add(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position, double price, size_t size) = 0;

	virtual void			Update(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position, double price, size_t size) = 0;

	virtual void			Delete(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position) = 0;


	// 在更新数据库之前需要把行情表和深度行情表合并
	virtual void			Combine(CodeHashId codeId, IBBidAsks& bidasks) = 0;



};

typedef std::shared_ptr<CMarketDepth> MarketDepthPtr;