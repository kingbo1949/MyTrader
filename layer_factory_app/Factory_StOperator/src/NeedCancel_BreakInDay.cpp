#include "pch.h"
#include "NeedCancel_BreakInDay.h"

CNeedCancel_BreakInDay::CNeedCancel_BreakInDay(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{

}

bool CNeedCancel_BreakInDay::CanCancelOpen()
{
	return false;
}

bool CNeedCancel_BreakInDay::CanCancelCover()
{
	return false;
}
