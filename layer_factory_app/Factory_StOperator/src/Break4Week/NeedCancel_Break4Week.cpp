//
// Created by kingb on 2026/1/25.
//

#include "NeedCancel_Break4Week.h"

CNeedCancel_Break4Week::CNeedCancel_Break4Week(const SubModuleParams &stParams)
    :CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_Break4Week::CanCancelOpen()
{
    return false;
}

bool CNeedCancel_Break4Week::CanCancelCover()
{
    return false;
}
