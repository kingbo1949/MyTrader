#include "pch.h"
#include "AnalystSimpleMatch_Real.h"
#include <Reverse.h>
#include <Factory_HashEnv.h>
#include <Factory_QDatabase.h>
#include "AnalystResult_Real.h"

AnalystResultPtr CAnalystSimpleMatch_Real::Go(const SimpleMatchPtrs& matchs)
{
	if (matchs.empty()) return nullptr;

	AnalystResultPtr back = std::make_shared<CAnalystResult_Real>();

	back->strategyId = matchs[0]->strategyKey.ToStr();
	Count_Profit(matchs, back->profit, back->profitRate);
	back->match_count = matchs.size();
	back->profitPerMatch = Count_ProfitPerMatch(back->profit, back->match_count);
	back->profitRatePerMatch = Count_ProfitRatePerMatch(back->profitRate, back->match_count);

	Count_WinAndLoss(matchs, back->winCount, back->lossCount, back->winRate);

	back->seriesLoss = Count_SeriesLoss(matchs);

	Count_WinAndLossMatchs(matchs, back->winMatchs, back->lossMatchs);

	back->maxDropDown = Count_MaxDropDown(matchs);

	back->profitRiskRate = back->profitRate / fabs(back->maxDropDown);
	
	return back;

}


void CAnalystSimpleMatch_Real::Count_Profit(const SimpleMatchPtrs& matchs, double& totalProfit, double& totalProfitRate)
{
	totalProfit = 0.0;
	totalProfitRate = 0.0;

	for (auto onematch : matchs)
	{
		totalProfit += onematch->profit;
		totalProfitRate += onematch->profitRate;
	}

	return ;

}

double CAnalystSimpleMatch_Real::Count_ProfitPerMatch(double totalProfit, size_t count)
{
	if (count != 0)
	{
		return totalProfit / count;
	}
	else
	{
		return 0.0;
	}

}

double CAnalystSimpleMatch_Real::Count_ProfitRatePerMatch(double totalProfitRate, size_t count)
{
	if (count != 0)
	{
		return totalProfitRate / count;
	}
	else
	{
		return 0.0;
	}

}

void CAnalystSimpleMatch_Real::Count_WinAndLoss(const SimpleMatchPtrs& matchs, size_t& winCount, size_t& lossCount, double& winRate)
{
	winCount = 0;
	lossCount = 0;
	winRate = 0.0;
	if (matchs.empty()) return;

	for (auto onematch : matchs)
	{
		if (onematch->profit > 0.0)
		{
			winCount++;
		}
		else
		{
			lossCount++;
		}

	}
	winRate = double(winCount) / matchs.size();
	return;

}

SimpleMatchPtrs CAnalystSimpleMatch_Real::Count_SeriesLoss(const SimpleMatchPtrs& matchs)
{
	SimpleMatchPtrs back;
	SimpleMatchPtrs lossMatchsInLoop;	// 当前在循环中处理的连续亏损

	for (auto onematch : matchs)
	{
		if (onematch->profit > 0.0)
		{
			if (lossMatchsInLoop.size() > back.size())
			{
				lossMatchsInLoop.swap(back);
			}
			lossMatchsInLoop.clear();
		}
		else
		{
			lossMatchsInLoop.push_back(onematch);
		}

		

	}

	if (lossMatchsInLoop.size() > back.size())
	{
		return lossMatchsInLoop;
	}
	else
	{
		return back;
	}

}

bool GreaterSort_SimpleMatch(SimpleMatchPtr a, SimpleMatchPtr b)
{
	return a->profitRate > b->profitRate;
}

bool LessSort_SimpleMatch(SimpleMatchPtr a, SimpleMatchPtr b)
{
	return a->profitRate < b->profitRate;
}

void CAnalystSimpleMatch_Real::Count_WinAndLossMatchs(const SimpleMatchPtrs& matchs, SimpleMatchPtrs& winMatchs, SimpleMatchPtrs& lossMatchs)
{
	for (const auto& onematch : matchs)
	{
		if (onematch->profit > 0.0)
		{
			winMatchs.push_back(onematch);
		}
		else
		{
			lossMatchs.push_back(onematch);
		}
	}
	// 对winMatchs降序
	std::sort(winMatchs.begin(), winMatchs.end(), GreaterSort_SimpleMatch);

	// 对lossMatchs升序
	std::sort(lossMatchs.begin(), lossMatchs.end(), LessSort_SimpleMatch);
	return;
}

double CAnalystSimpleMatch_Real::Count_MaxDropDown(const SimpleMatchPtrs& matchs)
{
	double currentMaxSumProfitRate = 0.0;	// 当前最高累计盈利，曾经达到过的最巅峰（用于遍历）
	double currentMaxDropDown = 0.0;		// 当前最大回撤（用于遍历）

	for (auto onematch : matchs)
	{
		if (onematch->sumProfitRate > currentMaxSumProfitRate)
		{
			// 创新高
			currentMaxSumProfitRate = onematch->sumProfitRate;
			continue;
		}

		double dropDown = (currentMaxSumProfitRate - onematch->sumProfitRate) * -1;
		if (dropDown > currentMaxDropDown) continue;

		currentMaxDropDown = dropDown;
	}
	return currentMaxDropDown;
}

