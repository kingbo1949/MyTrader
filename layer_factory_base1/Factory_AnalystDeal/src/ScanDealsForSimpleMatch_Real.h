#pragma once
#include "ScanDealsForSimpleMatch.h"
class CScanDealsForSimpleMatch_Real : public CScanDealsForSimpleMatch
{
public:
	CScanDealsForSimpleMatch_Real() { ; };
	virtual ~CScanDealsForSimpleMatch_Real() { ; };

	// 读取成交json文件，并按策略名归类回合
	virtual std::map<StrategyId, SimpleMatchPtrs>	Make_SimpleMatchs_FromJson() override final;

protected:
	std::map<StrategyId, SimpleMatchPtrs>			m_matchs;
	SimpleMatchPtr									m_currentMatch;

	// 按策略分成交单
	std::map<StrategyId, OneDealPtrs>				Split(OneDealPtrs deals);

	void											UpdateOneDeal_Open(OneDealPtr pdeal);
	void											UpdateOneDeal_Cover(OneDealPtr pdeal);

	// 计算并填写回合盈亏
	void											FillWinLoss(SimpleMatchPtr simpleMatch);

	// 判断是否子委托的成交 如果StrategyIdHashId中的CODE与deal中的code不同，即是子委托
	bool											IsChildDeal(OneDealPtr pdeal);

	// 扫描填写match的sumProfitRate字段
	void											ScanAndFill_SumProfitRate();
	void											ScanAndFill_SumProfitRate(SimpleMatchPtrs matchs);

	// 根据委托号码得到委托时间
	time_t											GetOrderTime(LocalOrderNo localOrderNo);

};

