#include "pch.h"
#include "NeedCancel_On29min.h"

CNeedCancel_On29min::CNeedCancel_On29min(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{
}

bool CNeedCancel_On29min::CanCancelOpen()
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

bool CNeedCancel_On29min::CanCancelCover()
{
	// n秒挂单不在最前面则撤单
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
