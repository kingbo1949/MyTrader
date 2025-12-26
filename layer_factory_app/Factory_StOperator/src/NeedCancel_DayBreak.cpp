#include "pch.h"
#include "NeedCancel_DayBreak.h"

CNeedCancel_DayBreak::CNeedCancel_DayBreak(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_DayBreak::CanCancelOpen()
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

bool CNeedCancel_DayBreak::CanCancelCover()
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

