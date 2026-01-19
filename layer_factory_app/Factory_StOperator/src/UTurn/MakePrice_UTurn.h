#pragma once

//
// DivType出现uturn，而且dif需要足够小，做多
// DivType出现uturn，而且dif需要足够大，做空
//

#include "../MakePrice_Real.h"


class CMakePrice_UTurn : public CMakePrice_Real
{
public:
    explicit CMakePrice_UTurn(const SubModuleParams& stParams);
    ~CMakePrice_UTurn() override { ; };

protected:
    bool			GetOpenPrice(double& price, RealPriceType& priceType) final;

    bool			GetCoverPrice(double& price, RealPriceType& priceType) final;


};

