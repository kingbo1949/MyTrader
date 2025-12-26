#include "pch.h"
#include "WhiteBoard_Real.h"
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <Factory_Log.h>
#include <Factory_IBJSon.h>
#include "Factory_QDatabase.h"
CWhiteBoard_Real::CWhiteBoard_Real()
{
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	for (auto contract : contracts)
	{
		CodeHashId hashId = Get_CodeIdEnv()->Get_CodeId_Hash(contract->codeId.c_str());
		IBTickPtr onetick = std::make_shared<CIBTick>();
		IBBidAsk bidask;
		onetick->bidAsks.push_back(bidask);
		onetick->codeHash = hashId;
		m_board[hashId] = onetick;
		printf("\n CWhiteBoard_Real bidask size = %d \n", int(onetick->bidAsks.size()));
	}
	return;
}

void CWhiteBoard_Real::Init()
{
	RangePair rangePair = Get_CodeIdEnv()->Get_CodeId_Range();
	for (size_t i = rangePair.beginPos; i <= rangePair.endPos; ++i)
	{
		Init(i);
	}
}

void CWhiteBoard_Real::Init(CodeHashId codeId)
{
	bool istrading = IsInTradeStatus(codeId, Get_CurrentTime()->GetCurrentTime_millisecond());
	if (!IsInTradeStatus(codeId, Get_CurrentTime()->GetCurrentTime_millisecond())) return;

	// 已开盘，从数据库中取初始化tick
	IBTickPtr ptick = MakeAndGet_QDatabase()->GetMarketQuoteDL(Get_CodeIdEnv()->Get_CodeStrByHashId(codeId));
	if (!ptick)
	{
		Log_Print(LogLevel::Warn, fmt::format("{} have no tick data in db for whiteBoard",
			Get_CodeIdEnv()->Get_CodeStrByHashId(codeId)
		));
		return;
	}
	TrimTick(ptick);


	istrading = IsInTradeStatus(codeId, ptick->time);
	Log_Print(LogLevel::Warn, fmt::format("WhiteBoard_Real::Init, {} IsInTradeStatus = {}, time = {}",
		Get_CodeIdEnv()->Get_CodeStrByHashId(codeId),
		istrading, CGlobal::GetTickTimeStr(ptick->time).c_str()));
	if (!IsInTradeStatus(codeId, ptick->time)) return;

	Log_Print(LogLevel::Info, fmt::format("Init form db, {} open = {}, last = {}, {}->{}",
		Get_CodeIdEnv()->Get_CodeStrByHashId(codeId),
		ptick->open, ptick->last, ptick->bidAsks[0].bid, ptick->bidAsks[0].ask
	));
	m_board[codeId] = ptick;

	printf("\n init bidask size = %d \n", int(ptick->bidAsks.size()));

	return;
}

IBTickPtr CWhiteBoard_Real::GetTick(CodeHashId codeId)
{
	return m_board[codeId];
}

bool CWhiteBoard_Real::IsInTradeStatus(CodeHashId codeId, time_t millisecondIn)
{
	// 在早开盘和下午收盘之间，属于开盘状态
	CTradePoint tradePoint = MakeAndGet_TradePointEnv()->GetTrandPoint(codeId);

	time_t dayMillisec = CHighFrequencyGlobalFunc::GetDayMillisec();

	time_t tpMillisec_begin = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.openMorning.c_str(), 0);
	time_t tpMillisec_end = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.closeAfternoon.c_str(), 0);

	if (millisecondIn < tpMillisec_begin || millisecondIn > tpMillisec_end) return false;

	return true;
}

bool CWhiteBoard_Real::ValidTick(IBTickPtr tick)
{
	if (tick->time == 0)
	{
		//Log_Print(LogLevel::Err, "time err");
		return false;
	}
	if (tick->open == 0)
	{
		//Log_Print(LogLevel::Err, "open err");
		return false;
	}
	if (tick->last == 0)
	{
		//Log_Print(LogLevel::Err, "last err");
		return false;
	}
	if (tick->bidAsks[0].bid == 0)
	{

		//Log_Print(LogLevel::Err, "bid err");
		return false;
	}
	if (tick->bidAsks[0].ask == 0)
	{
		//Log_Print(LogLevel::Err, "ask err");
		return false;
	}
	if (tick->bidAsks[0].bid >= tick->bidAsks[0].ask)
	{
		// Log_Print(LogLevel::Err, "bid ask err");
		return false;
	}

	return true;
}

void CWhiteBoard_Real::TrimTick(IBTickPtr tick)
{
	if (!tick) return;

	IBBidAsks bidasks;
	IBBidAsk bidask;
	bidask.bid = tick->bidAsks[0].bid;
	bidask.bidVol = tick->bidAsks[0].bidVol;
	bidask.ask = tick->bidAsks[0].ask;
	bidask.askVol = tick->bidAsks[0].askVol;
	bidasks.push_back(bidask);
	tick->bidAsks = bidasks;

	return;
}


void CWhiteBoard_Real::UpdateOpen(CodeHashId codeId, double open)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	if (m_board[codeId]->open != 0) return;

	m_board[codeId]->time = Get_CurrentTime()->GetCurrentTime_millisecond();
	m_board[codeId]->open = CGlobal::DToI(open / contract->minMove);

	//string str = fmt::format("{} rec open = {:.2f}", Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), open);
	//Log_Print(LogLevel::Info, str.c_str());
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

	return;
}

void CWhiteBoard_Real::UpdateBid(CodeHashId codeId, double bidprice)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	m_board[codeId]->bidAsks[0].bid = CGlobal::DToI(bidprice / contract->minMove);

	//string str = fmt::format("{} rec bidprice = {:.2f}", Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), bidprice);
	//Log_Print(LogLevel::Info, str.c_str());
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

}


void CWhiteBoard_Real::UpdateAsk(CodeHashId codeId, double askprice)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	m_board[codeId]->bidAsks[0].ask = CGlobal::DToI(askprice / contract->minMove);

	//string str = fmt::format("{} rec askprice = {:.2f}", Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), askprice);
	//Log_Print(LogLevel::Info, str.c_str());
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

}

IBTickPtr CWhiteBoard_Real::UpdateBidVol(CodeHashId codeId, int bidVol)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	IBTickPtr tick = m_board[codeId];

	tick->bidAsks[0].bidVol = bidVol;
	tick->time = Get_CurrentTime()->GetCurrentTime_millisecond();
	tick->timeStamp = Get_CurrentTime()->GetCurrentTime_nanoseconds();

	//string str = fmt::format("{} rec bidVol = {} and make tick success",
	//	Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), bidVol);
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

	//Log_Print(LogLevel::Info, str.c_str());

	if (!ValidTick(tick)) return nullptr;

	return tick->Clone();

}

IBTickPtr CWhiteBoard_Real::UpdateAskVol(CodeHashId codeId, int askVol)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	IBTickPtr tick = m_board[codeId];

	tick->bidAsks[0].askVol = askVol;
	tick->time = Get_CurrentTime()->GetCurrentTime_millisecond();
	tick->timeStamp = Get_CurrentTime()->GetCurrentTime_nanoseconds();

	//string str = fmt::format("{} rec askVol = {} and make tick success",
	//	Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), askVol);
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

	//Log_Print(LogLevel::Info, str.c_str());

	if (!ValidTick(tick)) return nullptr;

	return tick->Clone();

}

IBTickPtr CWhiteBoard_Real::UpdateTotalVol(CodeHashId codeId, int totalvol)
{
	IBTickPtr tick = m_board[codeId];
	tick->time = Get_CurrentTime()->GetCurrentTime_millisecond();
	tick->timeStamp = Get_CurrentTime()->GetCurrentTime_nanoseconds();

	if (tick->totalVol > totalvol) return nullptr;

	tick->totalVol = totalvol;

	if (!ValidTick(tick)) return nullptr;

	return tick->Clone();
}

void CWhiteBoard_Real::UpdateLastPrice(CodeHashId codeId, double last)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	m_board[codeId]->last = CGlobal::DToI(last / contract->minMove);
	m_board[codeId]->time = Get_CurrentTime()->GetCurrentTime_millisecond();

	if (m_board[codeId]->open == 0 && IsInTradeStatus(codeId, m_board[codeId]->time))
	{
		// 开盘之后，如果open没有初始化，则用last初始化它
		m_board[codeId]->open = m_board[codeId]->last;
	}

	//string str = fmt::format("{} rec last = {:.2f}", Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), last);
	//Log_Print(LogLevel::Info, str.c_str());
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

}

IBTickPtr CWhiteBoard_Real::UpdateLastVol(CodeHashId codeId, int lastVol)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	IBTickPtr tick = m_board[codeId];

	tick->vol = lastVol;
	tick->time = Get_CurrentTime()->GetCurrentTime_millisecond();
	tick->timeStamp = Get_CurrentTime()->GetCurrentTime_nanoseconds();

	//string str = fmt::format("{} rec lastvol = {} and make tick success",
	//	Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), lastVol);
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

	//Log_Print(LogLevel::Info, str.c_str());

	if (!ValidTick(tick)) return nullptr;

	return tick->Clone();

}


IBTickPtr CWhiteBoard_Real::UpdateBidAsk(CodeHashId codeId, double bid, int bidvol, double ask, int askvol)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	IBTickPtr tick = m_board[codeId];

	tick->bidAsks[0].bid = CGlobal::DToI(bid / contract->minMove);
	tick->bidAsks[0].bidVol = bidvol;
	tick->bidAsks[0].ask = CGlobal::DToI(ask / contract->minMove);
	tick->bidAsks[0].askVol = askvol;
	tick->time = Get_CurrentTime()->GetCurrentTime_millisecond();
	tick->timeStamp = Get_CurrentTime()->GetCurrentTime_nanoseconds();

	std::string str = fmt::format("{} rec bid = {:.2f}, bidvol = {}, ask = {:.2f}, askvol = {}, make tick success",
		Get_CodeIdEnv()->Get_CodeStrByHashId(codeId), bid, bidvol, ask, askvol);
	//Log_AsyncPrintDailyFile("whiteboard", str, 1, false);
	Log_Print(LogLevel::Info, str.c_str());

	if (!ValidTick(tick)) return nullptr;

	return tick->Clone();

}
