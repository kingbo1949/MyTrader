#include "pch.h"
#include "ScanDealsForSimpleMatch_Real.h"
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_Log.h>
#include <Factory_QDatabase.h>
#include <Global.h>
#include <Factory_TShareEnv.h>
std::map<StrategyId, SimpleMatchPtrs> CScanDealsForSimpleMatch_Real::Make_SimpleMatchs_FromJson()
{
	OneDealPtrs deals;
	MakeAndGet_JSonDeal()->Load_Deals(deals);

	std::map<StrategyId, OneDealPtrs> id_deals = Split(deals);

	for (auto oneitem : id_deals)
	{
		m_currentMatch = nullptr;

		for (auto onedeal : oneitem.second)
		{
			if (onedeal->openOrCover == RoundAction::Open_Round)
			{
				UpdateOneDeal_Open(onedeal);
			}
			else
			{
				UpdateOneDeal_Cover(onedeal);
			}
		}
	}

	ScanAndFill_SumProfitRate();

	return m_matchs;
}

std::map<StrategyId, OneDealPtrs> CScanDealsForSimpleMatch_Real::Split(OneDealPtrs deals)
{
	std::map<StrategyId, OneDealPtrs> ret;
	for (auto onedeal : deals)
	{
		if (IsChildDeal(onedeal)) continue;;

		StrategyParamPtr strategy = Get_StrategyParamEnv()->Get_StrategyParam(onedeal->key.strategyIdHashId);

		std::map<StrategyId, OneDealPtrs>::iterator pos = ret.find(strategy->key.ToStr());
		if (pos == ret.end())
		{
			OneDealPtrs id_deals;
			id_deals.insert(onedeal);
			ret[strategy->key.ToStr()] = id_deals;
		}
		else
		{
			pos->second.insert(onedeal);
		}
	}
	return ret;
}

void CScanDealsForSimpleMatch_Real::UpdateOneDeal_Open(OneDealPtr pdeal)
{
	StrategyParamPtr strategy = Get_StrategyParamEnv()->Get_StrategyParam(pdeal->key.strategyIdHashId);

	if (m_currentMatch)
	{
		Log_Print(LogLevel::Err, "open deal and m_currentMatch is not null");
		exit(-1);
	}
	m_currentMatch = std::make_shared<CSimpleMatch>();
	m_currentMatch->strategyKey = strategy->key;
	m_currentMatch->quantity = CGlobal::DToI(pdeal->dealQuantity);

	m_currentMatch->price_open = pdeal->dealPrice;
	m_currentMatch->dealtime_open = pdeal->dealTickTime;
	OneOrderPtr porder = GetOrderFromActiveOrInactive(pdeal->localOrderNo);
	if (porder)
	{
		m_currentMatch->ordertime_open = porder->tickTime;
	}
	else
	{
		// Gui分析交易记录时，可能MakeAndGet_DbActiveOrder与MakeAndGet_DbInactiveOrder没有启动，就会导致这种情况
		m_currentMatch->ordertime_open = m_currentMatch->dealtime_open;
	}




	return;

}

void CScanDealsForSimpleMatch_Real::UpdateOneDeal_Cover(OneDealPtr pdeal)
{
	StrategyParamPtr strategy = Get_StrategyParamEnv()->Get_StrategyParam(pdeal->key.strategyIdHashId);

	if (!m_currentMatch)
	{
		Log_Print(LogLevel::Err, "cover deal and m_currentMatch is null");
		exit(-1);
	}
	if (m_currentMatch->strategyKey.ToStr() != strategy->key.ToStr())
	{
		Log_Print(LogLevel::Err, "key is not equal");
		exit(-1);

	}
	if (m_currentMatch->quantity != pdeal->dealQuantity)
	{
		Log_Print(LogLevel::Err, "quantity is not equal");
		exit(-1);

	}

	m_currentMatch->price_cover = pdeal->dealPrice;
	m_currentMatch->dealtime_cover = pdeal->dealTickTime;
	OneOrderPtr porder = GetOrderFromActiveOrInactive(pdeal->localOrderNo);
	if (porder)
	{
		m_currentMatch->ordertime_cover = porder->tickTime;
	}
	else
	{
		m_currentMatch->ordertime_cover = m_currentMatch->dealtime_cover;
	}
	

	// 计算盈亏
	FillWinLoss(m_currentMatch);

	// 准备添加到map
	std::map<StrategyId, SimpleMatchPtrs>::iterator pos = m_matchs.find(strategy->key.ToStr());
	if (pos == m_matchs.end())
	{
		SimpleMatchPtrs matchs;
		matchs.push_back(m_currentMatch);
		m_matchs[strategy->key.ToStr()] = matchs;
	}
	else
	{
		pos->second.push_back(m_currentMatch);
	}

	m_currentMatch = nullptr;
	return;

}

void CScanDealsForSimpleMatch_Real::FillWinLoss(SimpleMatchPtr simpleMatch)
{
	if (simpleMatch->strategyKey.buyOrSell == BuyOrSell::Buy)
	{
		simpleMatch->profit = (simpleMatch->price_cover - simpleMatch->price_open) * simpleMatch->quantity;
	}
	else
	{
		simpleMatch->profit = (simpleMatch->price_open - simpleMatch->price_cover) * simpleMatch->quantity;
	}
	simpleMatch->profitRate = simpleMatch->profit / simpleMatch->price_open;
}

bool CScanDealsForSimpleMatch_Real::IsChildDeal(OneDealPtr pdeal)
{
	StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(pdeal->key.strategyIdHashId);
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(pdeal->codeHashId);

	if (pStrategyParam->key.targetCodeId != contract->codeId) return true;

	return false;
}

void CScanDealsForSimpleMatch_Real::ScanAndFill_SumProfitRate()
{
	for (auto item : m_matchs)
	{
		ScanAndFill_SumProfitRate(item.second);
	}
}

void CScanDealsForSimpleMatch_Real::ScanAndFill_SumProfitRate(SimpleMatchPtrs matchs)
{
	double currentSum = 0.0;
	for (auto onematch : matchs)
	{
		currentSum += onematch->profitRate;
		onematch->sumProfitRate = currentSum;
	}
}

time_t CScanDealsForSimpleMatch_Real::GetOrderTime(LocalOrderNo localOrderNo)
{
	return GetOrderFromActiveOrInactive(localOrderNo)->tickTime;
}

