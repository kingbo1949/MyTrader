//
// Created by kingb on 2026/1/10.
//

#include "NeedCancel_UTurn.h"

CNeedCancel_UTurn::CNeedCancel_UTurn(const SubModuleParams &stParams)
    :CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_UTurn::CanCancelOpen()
{
    return false;
}

bool CNeedCancel_UTurn::CanCancelCover()
{
    return false;
}
