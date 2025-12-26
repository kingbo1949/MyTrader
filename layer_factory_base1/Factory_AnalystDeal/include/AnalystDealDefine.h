#pragma once

#include <base_trade.h>

// 简单回合结构 开平仓手数一样多，一一对应
class CSimpleMatch
{
public:
	Strategykey			strategyKey;			// 策略
	int					quantity = 0;			// 成交手数（开平仓手数是一样的）

	double				price_open = 0;			// 开仓价格
	Tick_T				ordertime_open = 0;		// 开仓单委托时间
	Tick_T				dealtime_open = 0;		// 开仓单成交时间

	double				price_cover = 0;		// 平仓价格
	Tick_T				ordertime_cover = 0;	// 平仓单委托时间
	Tick_T				dealtime_cover = 0;		// 平仓单成交时间

	double				profit = 0.0;			// 盈亏（按绝对值计算）
	double				profitRate = 0.0;		// 盈亏（按收益率计算）
	double				sumProfitRate = 0.0;	// 累计盈亏（按收益率计算）

};
typedef std::shared_ptr<CSimpleMatch> SimpleMatchPtr;
typedef std::vector<SimpleMatchPtr> SimpleMatchPtrs;



class CAnalystResult
{
public:
	StrategyId			strategyId = "";
	size_t				match_count = 0;			// 回合次数
	double				profit = 0.0;				// 总盈利
	double				profitPerMatch = 0.0;		// 单笔盈利
	double				profitRate = 0.0;			// 收益率总盈利
	double				profitRatePerMatch = 0.0;	// 单笔收益率

	//double				odds = 0.0;					// 赔率 = 全部盈利交易的单笔平均盈利/全部亏损交易的单笔平均亏损
	//double				kellyRate = 0.0;			// 按凯利公式计算建议的头寸大小


	size_t				winCount = 0;			// 盈利次数
	size_t				lossCount = 0;			// 亏损次数
	double				winRate = 0.0;			// 盈利比率
	double				maxDropDown = 0.0;		// 最大回撤
	double				profitRiskRate = 0.0;	// 报酬风险比 = profitRate / abs(maxDropDown)

	SimpleMatchPtrs		seriesLoss;				// 最大连续亏损

	SimpleMatchPtrs		winMatchs;				// 盈利回合 按照收益率降序
	SimpleMatchPtrs		lossMatchs;				// 亏损回合 按照收益率升序

	virtual	void		Print_Simple() = 0;		// 简略打印

	// 打印到文件
	virtual	void		PrintToFile(const std::string& fileName, const SimpleMatchPtrs& matchs, bool needDealTime) = 0;

};
typedef std::shared_ptr<CAnalystResult> AnalystResultPtr;






