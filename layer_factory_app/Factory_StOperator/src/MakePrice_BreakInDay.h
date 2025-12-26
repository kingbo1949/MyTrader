#pragma once

// 日内20根K线突破开仓，10根K线反向突破则平仓
// 收盘前平仓

#include "MakePrice_Real.h"
class CMakePrice_BreakInDay : public CMakePrice_Real
{
public:
	CMakePrice_BreakInDay(const SubModuleParams& stParams);
	virtual ~CMakePrice_BreakInDay() { ; };

protected:
	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override final;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override final;

	bool					GetOpenCoverPrice(double& price);



};

