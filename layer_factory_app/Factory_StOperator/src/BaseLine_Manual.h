#pragma once
#include "BaseLine.h"

enum class LinePositon_Type
{
	Out,						// 两线之外，完全不符合条件
	Middle,						// 两线之间
	Deep						// 深度符合条件
};


class CBaseLine_Manual : public CBaseLine
{
public:
	CBaseLine_Manual(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint);
	virtual ~CBaseLine_Manual() { ; };

protected:

	IBKLinePtr						m_lastDay;		// 上一日的日线

	virtual bool					FreshPricePair(time_t now) override final;

	// m_pricePair已经存在，判断是否需要调整
	virtual bool					NeedFreshPricePair() override final;

	// 检查PricePair是否处于激活状态
	// 从启动时间开始计算，如果跌破过line，则多头激活
	// 从启动时间开始计算，如果涨破过line，则空头激活
	bool							CanActivatePricePair(const HighAndLow& highAndLowTradeIn);

	// 得到从启动时间开始到最近的盘中K线
	IBKLinePtrs						GetKLinesFromSetup();

	virtual double					TransParam(double param) override final;

	void							MakeLastDayKLine();

	void							SetPricePair(double openPrice, double stopPrice);
	void							SetPricePair(const HighAndLow& highAndLowTradeIn);

	LinePositon_Type				GetLinePositionType(double price);

	// 得到上一次交易的位置
	LinePositon_Type				GetLastOrderLinePositionType();

	// 得到当前价格处于的状态
	LinePositon_Type				GetLinePositionType(const HighAndLow& highAndLowTradeIn);




};

