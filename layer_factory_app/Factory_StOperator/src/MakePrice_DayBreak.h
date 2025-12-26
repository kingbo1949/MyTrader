#pragma once
#include "MakePrice_Real.h"
#include "BaseLine.h"

// 1. 选取一个高低价区间 如果useInterval = true 则区间是18:00:00 - 08:29:59高低价，
//    如果useInterval = false 则高低价都是08:30:00的开盘价格
// 2. 高低价区间选定之后，需要激活才能开始交易
//    区间的低点开盘后需要再跌param1，才能激活多头能力；区间高点再涨param1才能激活空头能力
// 3. 多头能力激活之后开始计算新低点，低点往上涨param2，开多头
//    空头能力激活之后开始计算新高点，高点往下跌param2，开空头


class CMakePrice_DayBreak : public CMakePrice_Real
{
public:
	CMakePrice_DayBreak(const SubModuleParams& stParams);
	virtual ~CMakePrice_DayBreak() { ; };

protected:
	time_t					m_openPoint;	// 基准时间，使用这个时间的开盘价格作为基准价格
	time_t					m_coverPoint;	// 基准时间，使用这个时间平仓

	BaseLinePtr				m_pBaseLine;


	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override;

	// 可以开仓的时间
	bool					IsCanOpenTime();

	// 得到持仓盈利价位
	int						GetProfitStep(PricePairPtr pricePair);

	// 按时间判断当前是否可以平仓
	// 对于NQ来说，如果盈利超过100，则当日的m_coverPoint不平仓，而是下一交易日在该时间平
	bool					IsCanCoverTime(PricePairPtr pricePair);

	MacdValue				GetMacd_LastM30();





};

