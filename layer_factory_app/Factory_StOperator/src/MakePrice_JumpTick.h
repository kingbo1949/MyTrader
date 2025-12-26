#pragma once
#include "MakePrice_Real.h"
class CMakePrice_JumpTick :  public CMakePrice_Real
{
public:
	CMakePrice_JumpTick(const SubModuleParams& stParams);
	virtual ~CMakePrice_JumpTick() { ; };

protected:

	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override final;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override final;

	// 开仓之后正常下平仓单
	bool					GetCoverPrice_AfterOpen(OneOrderPtr pOpenOrder, double& price, RealPriceType& priceType);

	// 平仓单被撤单之后重新下平仓单
	bool					GetCoverPrice_AfterCancel(double& price, RealPriceType& priceType);
};

