#include "pch.h"
#include "KLineStPriceMaker.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Global.h>
CKLineStPriceMaker::CKLineStPriceMaker(StrategyParamPtr pStrategyParam, const SubModuleParams& stParams, IBKLinePtr thisBar, IBKLinePtr lastBar)
	:m_pStrategyParam(pStrategyParam), m_stParams(stParams), m_thisKline(thisBar), m_lastKline(lastBar)
{
	m_codeId = m_stParams.contract->codeId;
	m_timetype = m_pStrategyParam->timetype;
	m_now = Get_CurrentTime()->GetCurrentTime_millisecond();
	
}

bool CKLineStPriceMaker::Make(double& price)
{
	MakeDiffStep();

	// 计算20根k线的极值m_highAndLow
	if (!MakeHighAndLow()) return 0;

	// 确定最后的极值m_extremeValue
	MakeExtremeValue();

	bool success = MakePriceByExtremePrice(price);
	if (!success)
	{
		
		UpdateMonitor();
	}
	
	return success;

}

bool CKLineStPriceMaker::MakeHighAndLow()
{
	size_t count = 20;
	IBKLinePtrs klines = CKLineAndMaTool::GetKLines_MoreDay(m_codeId, m_timetype, m_now, count);
	if (klines.size() != count) return false;

	m_highAndLow = CKLineAndMaTool::GetExtremePrice(klines);
	return true;

}

bool CKLineStPriceMaker::MakePriceByExtremePrice(double& price)
{
	if (m_stParams.buyOrSell == BuyOrSell::Buy)
	{
		if (m_thisKline->high < m_extremeValue - m_diff)
		{
			return false;
		}
		else
		{
			if (m_thisKline->open >= m_extremeValue - m_diff)
			{
				// 跳空高开
				price = m_thisKline->open;
			}
			else
			{
				price = m_extremeValue - m_diff;;
			}
		}
	}
	else
	{

		if (m_thisKline->low > m_extremeValue + m_diff)
		{
			return false;
		}
		else
		{
			if (m_thisKline->open <= m_extremeValue + m_diff)
			{
				// 跳空低开
				price = m_thisKline->open;
			}
			else
			{
				price = m_extremeValue + m_diff;
			}
		}
	}
	return true;

}
