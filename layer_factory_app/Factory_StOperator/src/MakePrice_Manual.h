#pragma once
#include "MakePrice_Real.h"
class CMakePrice_Manual : public CMakePrice_Real
{
public:
	CMakePrice_Manual(const SubModuleParams& stParams);
	virtual ~CMakePrice_Manual() { ; };

protected:
	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override final;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override final;

	bool					GetOpenCoverPrice(double& price);
	 

};

