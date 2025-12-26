#include "pch.h"
#include "NeedCancel_ReverseDayBreak.h"

CNeedCancel_ReverseDayBreak::CNeedCancel_ReverseDayBreak(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_ReverseDayBreak::CanCancelOpen()
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

bool CNeedCancel_ReverseDayBreak::CanCancelCover()
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

