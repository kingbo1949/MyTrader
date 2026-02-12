#pragma once
#include "../NeedCancel_Real.h"


class CNeedCancel_Break4Week : public CNeedCancel_Real
{
public:
    explicit CNeedCancel_Break4Week(const SubModuleParams& stParams);
    ~CNeedCancel_Break4Week() override { ; };


protected:

    bool			CanCancelOpen() final;

    bool			CanCancelCover() final;

};

