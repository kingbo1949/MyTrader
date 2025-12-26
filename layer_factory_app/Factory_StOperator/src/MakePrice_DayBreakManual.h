#pragma once

// 1. 通过line1， line2指定2条线作为基准
// 2. 基准线需要激活才能开始交易
//    跌破基准线，才能激活多头能力；涨破基准线才能激活空头能力
// 3. 多头能力激活之后开始计算新低点，低点往上涨param2，开多头
//    空头能力激活之后开始计算新高点，高点往下跌param2，开空头
// 4. 跌破line1之后，如果发送过开仓单，基准都不再变化，除非跌破line2才会再次计算基准，即canChgBaseCount参数在本策略无效


#include "MakePrice_Real.h"
#include "BaseLine.h"
class CMakePrice_DayBreakManual : public CMakePrice_Real
{
public:
	CMakePrice_DayBreakManual(const SubModuleParams& stParams);
	virtual ~CMakePrice_DayBreakManual() { ; };

protected:
	time_t					m_coverPoint;	// 基准时间，使用这个时间平仓

	BaseLinePtr				m_pBaseLine;


	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override;



};

