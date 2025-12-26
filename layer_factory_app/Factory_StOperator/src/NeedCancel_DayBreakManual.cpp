#include "pch.h"
#include "NeedCancel_DayBreakManual.h"

CNeedCancel_DayBreakManual::CNeedCancel_DayBreakManual(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_DayBreakManual::CanCancelOpen()
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

bool CNeedCancel_DayBreakManual::CanCancelCover()
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
