#include "pch.h"
#include "CheckMa.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include "KLineAndMaTool.h"
CCheckMa::CCheckMa(StrategyParamPtr pStrategyParam)
	:m_pStrategyParam(pStrategyParam), m_pMaResult(nullptr)
{
}

bool CCheckMa::Check(time_t ms)
{
	// 这个函数在8：30之后才会被调用，且只会成功地调用一次
	time_t dayTime = CHighFrequencyGlobalFunc::GetDayMillisec(ms);
	if (m_pMaResult && m_pMaResult->maDayTime == dayTime) return true;

	if (!FillMaResult_DayPart(ms)) return false;
	if (!FillMaResult_1HourPart(ms)) return false;

	return true;
}

double CCheckMa::GetLastDayClose()
{
	return m_pMaResult->klines_d1.back()->close;
}

double CCheckMa::GetLastDayHigh()
{
	return m_pMaResult->klines_d1.back()->high;
}

double CCheckMa::GetLastDayLow()
{
	return m_pMaResult->klines_d1.back()->low;
}

bool CCheckMa::FillMaResult_DayPart(time_t ms)
{
	size_t count = 5;
	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_MoreDay(m_pStrategyParam->key.targetCodeId, Time_Type::D1, ms, count);
	if (klines.size() != count) return false;

	//CHighFrequencyGlobalFunc::PrintKLines(klines);

	if (!m_pMaResult)
	{
		m_pMaResult = std::make_shared<MaResult>();
	}
	m_pMaResult->maDayTime = CHighFrequencyGlobalFunc::GetDayMillisec(ms);
	swap(m_pMaResult->klines_d1, klines);

	m_pMaResult->ma_d1_5 = CKLineAndMaTool::GetMa(m_pMaResult->klines_d1);
	return true;

}

bool CCheckMa::FillMaResult_1HourPart(time_t ms)
{
	size_t count = 20;
	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_MoreDay(m_pStrategyParam->key.targetCodeId, Time_Type::H1, ms, count);
	if (klines.size() != count) return false;
	m_pMaResult->ma_h1_20 = CKLineAndMaTool::GetMa(klines);

	count = 60;
	klines = CKLineAndMaTool::GetKLines_MoreDay(m_pStrategyParam->key.targetCodeId, Time_Type::H1, ms, count);
	if (klines.size() != count) return false;
	m_pMaResult->ma_h1_60 = CKLineAndMaTool::GetMa(klines);

	return true;
}


