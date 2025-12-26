#include "pch.h"
#include "NeedCancel_DoubleBreak.h"

CNeedCancel_DoubleBreak::CNeedCancel_DoubleBreak(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_DoubleBreak::CanCancelOpen()
{
	// N秒挂单不在最前面则撤单
	int second = 5;
	if (CheckRecentOrderOnTop(second))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool CNeedCancel_DoubleBreak::CanCancelCover()
{
	// N秒挂单不在最前面则撤单
	int second = 5;
	if (CheckRecentOrderOnTop(second))
	{
		return false;
	}
	else
	{
		return true;
	}
}
