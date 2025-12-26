#pragma once

// 本类遍历一个回合集合，返回一个AnalystResultPtr对象作为结果，单笔盈利 交易次数等重大的结果都在其中，AnalystResultPtr不再负责遍历计算，只负责显示

#include "AnalystSimpleMatch.h"
class CAnalystSimpleMatch_Real : public CAnalystSimpleMatch
{
public:
	CAnalystSimpleMatch_Real() { ; };
	virtual ~CAnalystSimpleMatch_Real() { ; };

	virtual AnalystResultPtr			Go(const SimpleMatchPtrs& matchs) override final;


protected:

	// 计算总盈利 总收益率
	void								Count_Profit(const SimpleMatchPtrs& matchs, double& totalProfit, double& totalProfitRate);

	// 计算单笔盈利 
	double								Count_ProfitPerMatch(double totalProfit, size_t count);

	// 计算单比收益率
	double								Count_ProfitRatePerMatch(double totalProfitRate, size_t count);

	// 计算盈利次数 亏损次数 盈亏比率
	void								Count_WinAndLoss(const SimpleMatchPtrs& matchs, size_t& winCount, size_t& lossCount, double& winRate);

	// 计算最多次数的连续亏损
	SimpleMatchPtrs						Count_SeriesLoss(const SimpleMatchPtrs& matchs);

	// 分离亏损和盈利回合
	void								Count_WinAndLossMatchs(const SimpleMatchPtrs& matchs, SimpleMatchPtrs& winMatchs, SimpleMatchPtrs& lossMatchs);

	// 计算最大回撤
	double								Count_MaxDropDown(const SimpleMatchPtrs& matchs);


};

