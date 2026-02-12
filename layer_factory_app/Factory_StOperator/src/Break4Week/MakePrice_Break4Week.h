#pragma once

//
// 突破20周期最高点，平空 做多
// 突破20周期最低点，平多 做空
//

#include "../MakePrice_Real.h"


class CMakePrice_Break4Week : public CMakePrice_Real
{
public:
    explicit CMakePrice_Break4Week(const SubModuleParams& stParams);
    ~CMakePrice_Break4Week() override { ; };

protected:
    bool			GetOpenPrice(double& price, RealPriceType& priceType) final;

    bool			GetCoverPrice(double& price, RealPriceType& priceType) final;

};

