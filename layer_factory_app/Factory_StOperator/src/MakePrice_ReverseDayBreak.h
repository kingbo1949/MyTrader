#pragma once
#include "MakePrice_DayBreak.h"
class CMakePrice_ReverseDayBreak : public CMakePrice_DayBreak
{
public:
	CMakePrice_ReverseDayBreak(const SubModuleParams& stParams);
	virtual ~CMakePrice_ReverseDayBreak() { ; };

protected:

	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override final;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override final;


};

