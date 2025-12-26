#pragma once
#include "MarketDepth.h"

struct MktRow
{
	bool			valid = false;
	int				priceStep = 0;
	size_t			size = 0;
};

struct BidOrAskRow
{
	int				priceStep = 0;
	size_t			size = 0;
};

class CMarketDepth_Real : public CMarketDepth
{
public:
	CMarketDepth_Real() { ; };
	virtual ~CMarketDepth_Real() { ; };

	virtual void			Init() override final { ; };

	virtual void			Add(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position, double price, size_t size)  override final;

	virtual void			Update(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position, double price, size_t size)  override final;

	virtual void			Delete(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position)  override final;


	// 在更新数据库之前需要把行情表和深度行情表合并
	virtual void			Combine(CodeHashId codeId, IBBidAsks& bidasks)  override final;

protected:
	std::mutex				m_mutex;
	MktRow					m_bid[CodeHashIdSize][MaxPriceRowCountSubscribe];
	MktRow					m_ask[CodeHashIdSize][MaxPriceRowCountSubscribe];


	MktRow& GetMktRow(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position);

	std::vector<BidOrAskRow>		Combine_Bid(CodeHashId codeId, const IBBidAsks& bidasks);
	std::vector<BidOrAskRow>		Combine_Ask(CodeHashId codeId, const IBBidAsks& bidasks);

	// 检查是否bid比ask还要更大
	void					CheckBidAsk(CodeHashId codeId);





};

