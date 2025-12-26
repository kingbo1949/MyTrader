#include "pch.h"
#include "MarketDepth_Real.h"
#include "Factory_QDatabase.h"
#include <Global.h>
#include <Factory_Log.h>
void CMarketDepth_Real::Add(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position, double price, size_t size)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	double minMove = MakeAndGet_ContractEnv()->GetContract(codeId)->minMove;
	MktRow& row = GetMktRow(codeId, bidOrAsk, position);
	row.valid = true;
	row.priceStep = CGlobal::DToI(price / minMove);
	row.size = size;
	return;
}

void CMarketDepth_Real::Update(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position, double price, size_t size)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	double minMove = MakeAndGet_ContractEnv()->GetContract(codeId)->minMove;
	MktRow& row = GetMktRow(codeId, bidOrAsk, position);
	row.valid = true;
	row.priceStep = CGlobal::DToI(price / minMove);
	row.size = size;

	// CheckBidAsk(codeId);
	return;
}

void CMarketDepth_Real::Delete(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	MktRow& row = GetMktRow(codeId, bidOrAsk, position);
	row.valid = false;
	return;
}

void CMarketDepth_Real::Combine(CodeHashId codeId, IBBidAsks& bidasks)
{
	if (bidasks.size() != 1)
	{
		printf("in bidasks size = %d \n", int(bidasks.size()));
		exit(-1);
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<BidOrAskRow> bid_rows = Combine_Bid(codeId, bidasks);
	std::vector<BidOrAskRow> ask_rows = Combine_Ask(codeId, bidasks);

	size_t rowSize = std::min(bid_rows.size(), ask_rows.size());
	for (size_t i = 0; i < rowSize; ++i)
	{
		IBBidAsk bidAsk;
		bidAsk.bid = bid_rows[i].priceStep;
		bidAsk.bidVol = bid_rows[i].size;
		bidAsk.ask = ask_rows[i].priceStep;
		bidAsk.askVol = ask_rows[i].size;
		bidasks.push_back(bidAsk);
	}
	return;
}

MktRow& CMarketDepth_Real::GetMktRow(CodeHashId codeId, BidOrAsk bidOrAsk, size_t position)
{
	if (bidOrAsk == BidOrAsk::Bid)
	{
		return m_bid[codeId][position];
	}
	else
	{
		return m_ask[codeId][position];
	}
}

std::vector<BidOrAskRow> CMarketDepth_Real::Combine_Bid(CodeHashId codeId, const IBBidAsks& bidasks)
{
	std::vector<BidOrAskRow> ret;   // 返回的ROW不包含盘口

	int benchPriceStep = bidasks[0].bid;

	for (size_t i = 0; i < MaxPriceRowCountSubscribe; ++i)
	{
		if (!m_bid[codeId][i].valid) continue;
		if (ret.size() != 0)
		{
			benchPriceStep = ret.back().priceStep;
		}

		if (m_bid[codeId][i].priceStep < benchPriceStep)
		{
			BidOrAskRow row;
			row.priceStep = m_bid[codeId][i].priceStep;
			row.size = m_bid[codeId][i].size;
			ret.push_back(row);
			if (ret.size() >= MaxPriceRowCountDb - 1) break;
		}
	}
	return ret;
}

std::vector<BidOrAskRow> CMarketDepth_Real::Combine_Ask(CodeHashId codeId, const IBBidAsks& bidasks)
{
	std::vector<BidOrAskRow> ret;   // 返回的ROW不包含盘口

	int benchPriceStep = bidasks[0].ask;

	for (size_t i = 0; i < MaxPriceRowCountSubscribe; ++i)
	{
		if (!m_ask[codeId][i].valid) continue;
		if (ret.size() != 0)
		{
			benchPriceStep = ret.back().priceStep;
		}

		if (m_ask[codeId][i].priceStep > benchPriceStep)
		{
			BidOrAskRow row;
			row.priceStep = m_ask[codeId][i].priceStep;
			row.size = m_ask[codeId][i].size;
			ret.push_back(row);
			if (ret.size() >= MaxPriceRowCountDb - 1) break;
		}
	}
	return ret;
}

void CMarketDepth_Real::CheckBidAsk(CodeHashId codeId)
{
	double minMove = MakeAndGet_ContractEnv()->GetContract(codeId)->minMove;
	MktRow& bidRow = GetMktRow(codeId, BidOrAsk::Bid, 0);
	MktRow& askRow = GetMktRow(codeId, BidOrAsk::Ask, 0);
	if (bidRow.valid && askRow.valid && bidRow.priceStep >= askRow.priceStep)
	{
		std::string str = fmt::format("CheckBidAsk error, bid = {:.2f}, ask = {:.2f}",
			bidRow.priceStep * minMove,
			askRow.priceStep * minMove
		);
		Log_Print(LogLevel::Critical, str);
	}
	return;

}
