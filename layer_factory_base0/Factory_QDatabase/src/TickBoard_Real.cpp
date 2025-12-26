#include "pch.h"
#include "TickBoard_Real.h"
#include <Factory_Log.h>
#include <Global.h>
#include "Factory_QDatabase.h"

CTickBoard_Real::CTickBoard_Real()
	:m_status(TickBoardStatus::Ready)
{

}

TickBoardStatus CTickBoard_Real::GetTickBoardStatus()
{
	return m_status;
}

void CTickBoard_Real::SetTickBoardStatus(TickBoardStatus status)
{
	m_status = status;
}

CodeHashId CTickBoard_Real::GetCodeHash()
{
	return m_codeHash;
}

void CTickBoard_Real::SetCodeHash(CodeHashId codeHash)
{
	m_codeHash = codeHash;
	return;
}

void CTickBoard_Real::AddOne(IBTickPtr tick)
{
	if (m_status != TickBoardStatus::Busy)
	{
		Log_Print(LogLevel::Err, "CTickBoard_Real::AddOne, m_status != TickBoardStatus::Busy!!");
		exit(-1);
	}

	m_ticks.push_back(tick);
	return;

}


IBTickPtrs CTickBoard_Real::ReceiveAll()
{
	if (m_status != TickBoardStatus::Done)
	{
		Log_Print(LogLevel::Err, "CTickBoard_Real::ReceiveAll, m_status != TickBoardStatus::Done!!");
		exit(-1);
	}
	// 整理数据
	RefillAllTicks();

	IBTickPtrs back;
	m_ticks.swap(back);

	SetTickBoardStatus(TickBoardStatus::Ready);

	return back;

}

void CTickBoard_Real::RefillAllTicks()
{
	// 使得bidAsk 与 last相差0.1元
	const double bidaskStep = 0.1;
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(GetCodeHash() );

	int totalvol = 0;
	for (auto tick : m_ticks)
	{
		tick->codeHash = GetCodeHash();

		// 开始伪造数据
		if (tick->bidAsks.empty())
		{
			IBBidAsk	bidAsk;
			bidAsk.bid = tick->last - CGlobal::DToI(bidaskStep / contract->minMove);
			bidAsk.bidVol = 1;
			bidAsk.ask = tick->last + CGlobal::DToI(bidaskStep / contract->minMove);
			bidAsk.askVol = 1;
			tick->bidAsks.push_back(bidAsk);
		}
		tick->open = tick->last;
		totalvol += tick->vol;
		tick->totalVol = totalvol;
	}
}

