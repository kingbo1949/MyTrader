#include "pch.h"
#include "TradePointEnv_Real.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_HashEnv.h>
#include <Factory_IBJSon.h>
#include <Factory_Log.h>
#include <Global.h>
#include "Factory_QDatabase.h"
CTradePointEnv_Real::CTradePointEnv_Real()
{
	TradePointPtrs tradePoints = MakeAndGet_JSonTradePoint()->Load_TradePoints();

	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);

	for (auto contract : contracts)
	{
		CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(contract->codeId.c_str());

		// 得到指定合约的CTradePoint
		CTradePoint tradePoint = GetTrandPoint(codeHash, tradePoints);

		m_tradePoints[codeHash] = tradePoint;
	}
	return;

}

CTradePoint CTradePointEnv_Real::GetTrandPoint(CodeHashId codeId)
{
	return m_tradePoints[codeId];
}

CTradePoint CTradePointEnv_Real::GetTrandPoint(CodeHashId codeId, const TradePointPtrs& allTimePoints)
{
	// 耗时操作，仅用于初始化
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);

	for (auto timePoint : allTimePoints)
	{
		if (contract->primaryExchangePl == timePoint->primaryExchangePl) return *timePoint;
	}

	CTradePoint back;
	Log_Print(LogLevel::Err, fmt::format("can not find timePoint: {} ", contract->codeId.c_str()));
	exit(-1);
	return back;

}

void CTradePointEnv_Real::CheckAndSetStatus()
{
	RangePair range = Get_CodeIdEnv()->Get_CodeId_Range();
	for (size_t i = range.beginPos; i <= range.endPos; ++i)
	{
		TradePointDec tpDec = GetTradePointDec(i, Get_CurrentTime()->GetCurrentTime_millisecond());
		m_tpDecs[i] = tpDec;
	}
	return;

}

TradePointDec CTradePointEnv_Real::GetTradePointDecResult(CodeHashId codeId)
{
	return m_tpDecs[codeId];
}

bool CTradePointEnv_Real::NearCloseForForceCover(CodeHashId codeId)
{
	bool back = true;
	TradePointDec dec = GetTradePointDecResult(codeId);

	if (dec.status == TradePointStatus::CloseAfternoon || dec.status == TradePointStatus::CloseEvening)
	{
		// 收盘前4分钟到收盘前2分钟，返回true，给了2分钟来处理平仓
		back = (dec.millisecond > -60 * 4 * 1000) && (dec.millisecond < -60 * 2 * 1000);
	}
	else
	{
		back = false;
	}
	return back;

}

bool CTradePointEnv_Real::NearClose(CodeHashId codeId)
{
	bool back = true;
	TradePointDec dec = GetTradePointDecResult(codeId);

	if (dec.status == TradePointStatus::CloseAfternoon || dec.status == TradePointStatus::CloseEvening)
	{
		// 收盘前5分钟返回true
		back = (dec.millisecond > -60 * 5 * 1000);
	}
	else
	{
		back = false;
	}
	return back;

}

TradePointDec CTradePointEnv_Real::GetTradePointDec(CodeHashId codeId, time_t millisecondIn) const
{
	const CTradePoint& tradePoint = m_tradePoints[codeId];

	TradePointDec back = GetTradePointDec(millisecondIn, TradePointStatus::OpenMorning, tradePoint);

	TradePointDec temDec = GetTradePointDec(millisecondIn, TradePointStatus::CloseMorning, tradePoint);
	back = CompareTPDec(back, temDec);

	temDec = GetTradePointDec(millisecondIn, TradePointStatus::OpenAfternoon, tradePoint);
	back = CompareTPDec(back, temDec);

	temDec = GetTradePointDec(millisecondIn, TradePointStatus::CloseAfternoon, tradePoint);
	back = CompareTPDec(back, temDec);

	temDec = GetTradePointDec(millisecondIn, TradePointStatus::OpenEvening, tradePoint);
	back = CompareTPDec(back, temDec);

	temDec = GetTradePointDec(millisecondIn, TradePointStatus::CloseEvening, tradePoint);
	back = CompareTPDec(back, temDec);

	temDec = GetTradePointDec(millisecondIn, TradePointStatus::BreakBegin, tradePoint);
	back = CompareTPDec(back, temDec);

	temDec = GetTradePointDec(millisecondIn, TradePointStatus::BreakEnd, tradePoint);
	back = CompareTPDec(back, temDec);

	return back;

}

TradePointDec CTradePointEnv_Real::GetTradePointDec(time_t millisecond, TradePointStatus status, const CTradePoint& tradePoint) const
{
	time_t dayMillisec = CHighFrequencyGlobalFunc::GetDayMillisec();

	// 时点status在当日的真实时间（毫秒）
	time_t tpMillisec = 0;
	switch (status)
	{
	case TradePointStatus::OpenMorning:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.openMorning.c_str(), 0);
		break;
	case TradePointStatus::CloseMorning:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.closeMorning.c_str(), 0);
		break;
	case TradePointStatus::OpenAfternoon:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.openAfternoon.c_str(), 0);
		break;
	case TradePointStatus::CloseAfternoon:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.closeAfternoon.c_str(), 0);
		break;
	case TradePointStatus::OpenEvening:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.openEvening.c_str(), 0);
		break;
	case TradePointStatus::CloseEvening:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.closeEvening.c_str(), 0);
		break;
	case TradePointStatus::BreakBegin:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.breakBegin.c_str(), 0);
		break;
	case TradePointStatus::BreakEnd:
		tpMillisec = dayMillisec + CHighFrequencyGlobalFunc::MakeMilliSecondPart(tradePoint.breakEnd.c_str(), 0);
		break;
	default:
		break;
	}

	TradePointDec back;
	back.status = status;
	back.millisecond = int(millisecond - tpMillisec);
	return back;

}


TradePointDec CTradePointEnv_Real::CompareTPDec(const TradePointDec& dec1, const TradePointDec& dec2) const
{
	if (abs(dec1.millisecond) < abs(dec2.millisecond)) return dec1;

	return dec2;

}

