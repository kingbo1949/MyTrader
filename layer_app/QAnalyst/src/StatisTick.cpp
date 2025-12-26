#include "stdafx.h"
#include "StatisTick.h"
#include <Log.h>
#include <Factory_Log.h>
#include <Global.h>
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
CStatisTick::CStatisTick(const CodeStr& codeId, const IBTickPtrs& ticks, int backupStep)
	:m_codeId(codeId), m_ticks(ticks), m_backupStep(backupStep)
{
	m_contract = MakeAndGet_ContractEnv()->GetContract(m_codeId);
}

void CStatisTick::ScanForJumpPoints(JumpPoints& bidJumps, JumpPoints& askJumps)
{
	IBTickPtr pLast = nullptr;
	for (auto i = 0; i < m_ticks.size(); ++i)
	{
		IBTickPtr tick = m_ticks[i];

		if (!pLast)
		{
			pLast = tick;
			continue;
		}
		int jumpStep = 8;
		if (tick->bidAsks[0].bid - pLast->bidAsks[0].bid >= jumpStep)
		{
			bidJumps.push_back(i);
		}
		if (pLast->bidAsks[0].ask - tick->bidAsks[0].ask >= jumpStep)
		{
			askJumps.push_back(i);
		}
		pLast = tick;
	}
}

void CStatisTick::FillJumpMap(Second_T second, const JumpPoints& jumps, BidOrAsk bidOrAskJump)
{
	for (auto index : jumps)
	{
		FillOneJumpMap(second, index, bidOrAskJump);
	}
}

void CStatisTick::PrintToFile(BidOrAsk bidOrAskJump)
{
	// 时间 high low close
	std::string fileName = "";
	if (bidOrAskJump == BidOrAsk::Bid)
	{
		fileName = "log/bidJumps.csv";
	}
	else
	{
		fileName = "log/askJumps.csv";
	}

	CLog::Instance()->DelLogFile(fileName);
	std::string tital = "time, high, low, close, vol, jumpStep, TrendType, HoldOn, ma5, diffMa5 \n";
	CLog::Instance()->PrintStrToFile(fileName, tital);

	TickJumpMap& jumpMap = m_bidJumps;
	if (bidOrAskJump == BidOrAsk::Ask)
	{
		jumpMap = m_askJumps;
	}
	std::string outputStr = "";
	for (auto item : jumpMap)
	{
		Tick_T time = m_ticks[item.first]->time;
		std::string str = fmt::format("{}, {}, {}, {}, {}, {}, {}, {}, {:.2f}, {:.2f}\n",
			CGlobal::GetTickTimeStr(time).c_str(),
			item.second.forwardHigh,
			item.second.backwardLow,
			item.second.endClose,
			item.second.vol,
			item.second.jumpStep,
			CTransToStr::Get_TrendType(item.second.trendType).c_str(),
			CTransToStr::Get_Bool(item.second.isHoldon).c_str(),
			item.second.ma5,
			item.second.diffMa5
		);
		outputStr += str;

	}
	CLog::Instance()->PrintStrToFile(fileName, outputStr);

}

void CStatisTick::FillOneJumpMap(Second_T second, VectorIndex jumpIndex, BidOrAsk bidOrAskJump)
{
	Tick_T endTickTime = m_ticks[jumpIndex]->time + second * 1000;

	MoveAfterJump oneMove;
	oneMove.vol = GetVol(jumpIndex);
	oneMove.jumpStep = GetJumpStep(jumpIndex, bidOrAskJump);
	oneMove.trendType = GetJumpTrendType(jumpIndex, bidOrAskJump);
	oneMove.isHoldon = GetHoldon(jumpIndex, bidOrAskJump);


	oneMove.ma5 = MakeAndGet_QDatabase()->MakeMa(m_codeId, Time_Type::S15, 5, m_ticks[jumpIndex]->time);
	oneMove.diffMa5 = GetBidOrAsk(m_ticks[jumpIndex], bidOrAskJump) * m_contract->minMove - oneMove.ma5;

	VectorIndex beginIndex = GetBeginIndexForBackupStep(jumpIndex, bidOrAskJump, endTickTime);
	if (beginIndex < 0)
	{
		if (bidOrAskJump == BidOrAsk::Bid)
		{
			m_bidJumps[jumpIndex] = oneMove;
		}
		else
		{
			m_askJumps[jumpIndex] = oneMove;
		}
		return;
	}


	TickStep beginStep = 0;
	for (auto i = beginIndex; i < m_ticks.size(); ++i)
	{
		IBTickPtr tick = m_ticks[i];
		if (i == beginIndex)
		{
			beginStep = GetBidOrAsk(tick, bidOrAskJump);
			continue;
		}

		if (m_ticks[i]->time >= endTickTime)
		{
			if (bidOrAskJump == BidOrAsk::Bid)
			{
				m_bidJumps[jumpIndex] = oneMove;
			}
			else
			{
				m_askJumps[jumpIndex] = oneMove;
			}
			return;
		}

		if (bidOrAskJump == BidOrAsk::Bid)
		{
			oneMove.endClose = tick->bidAsks[0].bid - beginStep;
		}
		else
		{
			oneMove.endClose = beginStep - tick->bidAsks[0].ask;
		}


		if (oneMove.endClose > oneMove.forwardHigh)
		{
			oneMove.forwardHigh = oneMove.endClose;
		}
		if (oneMove.endClose < oneMove.backwardLow)
		{
			oneMove.backwardLow = oneMove.endClose;
		}

	}
}

int CStatisTick::GetVol(VectorIndex jumpIndex)
{
	int tickVol = m_ticks[jumpIndex]->totalVol;
	for (auto i = jumpIndex - 1; i > 0; i--)
	{
		IBTickPtr tick = m_ticks[i];
		if (tick->totalVol != tickVol)
		{
			return tickVol - tick->totalVol;
		}
	}
	return 0;
}

int CStatisTick::GetBeginIndexForBackupStep(VectorIndex jumpIndex, BidOrAsk bidOrAskJump, Tick_T endTickTime)
{
	if (m_backupStep == 0) return jumpIndex;

	int beginStep = 0;
	int endStep = 0;
	if (bidOrAskJump == BidOrAsk::Bid)
	{
		beginStep = m_ticks[jumpIndex]->bidAsks[0].bid;
		endStep = beginStep - m_backupStep;
	}
	else
	{
		beginStep = m_ticks[jumpIndex]->bidAsks[0].ask;
		endStep = beginStep + m_backupStep;
	}


	for (auto i = jumpIndex + 1; i < m_ticks.size(); ++i)
	{
		IBTickPtr tick = m_ticks[i];
		if (tick->time >= endTickTime) break;

		if (bidOrAskJump == BidOrAsk::Bid)
		{
			if (tick->bidAsks[0].bid <= endStep) return i;
		}
		else
		{
			if (tick->bidAsks[0].ask >= endStep) return i;
		}

	}
	return -1;
}

int CStatisTick::GetJumpStep(VectorIndex jumpIndex, BidOrAsk bidOrAskJump)
{
	IBTickPtr pLast = m_ticks[jumpIndex - 1];
	IBTickPtr tick = m_ticks[jumpIndex];
	if (bidOrAskJump == BidOrAsk::Bid)
	{
		return tick->bidAsks[0].bid - pLast->bidAsks[0].bid;
	}
	else
	{
		return pLast->bidAsks[0].ask - tick->bidAsks[0].ask;
	}
}

TrendType CStatisTick::GetJumpTrendType(VectorIndex jumpIndex, BidOrAsk bidOrAskJump)
{


	IBTickPtr tick = m_ticks[jumpIndex];
	int circleFast = 5;
	int circleSlow = 10;
	CheckTrendPtr pCheckTrend = MakeAndGet_CheckTrend(m_codeId, Time_Type::S15, circleFast, circleSlow, tick->time);
	TrendType ret;
	if (bidOrAskJump == BidOrAsk::Bid)
	{
		pCheckTrend->GetTrendType(tick->last * m_contract->minMove, LongOrShort::LongT, ret);
	}
	else
	{
		pCheckTrend->GetTrendType(tick->last * m_contract->minMove, LongOrShort::ShortT, ret);
	}
	return ret;

}

bool CStatisTick::GetHoldon(VectorIndex jumpIndex, BidOrAsk bidOrAskJump)
{

	IBTickPtr tick = m_ticks[jumpIndex];
	if (bidOrAskJump == BidOrAsk::Ask)
	{
		if (tick->bidAsks[0].bid > tick->last) return true;
	}
	else
	{
		if (tick->bidAsks[0].ask < tick->last) return true;
	}

	return false;
}

int CStatisTick::GetBidOrAsk(IBTickPtr tick, BidOrAsk bidOrAsk)
{
	if (bidOrAsk == BidOrAsk::Bid)
	{
		return tick->bidAsks[0].bid;
	}
	else
	{
		return tick->bidAsks[0].ask;
	}
}

