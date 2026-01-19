#include "../pch.h"
#include "NeedCancel_Manual.h"
#include <Factory_UnifyInterface.h>
CNeedCancel_Manual::CNeedCancel_Manual(const SubModuleParams& stParams)
	:CNeedCancel_Real(stParams)
{

}

bool CNeedCancel_Manual::CanCancelOpen()
{
	if (Get_Manual_Status() == ManualStatus::CancelManul) return true;

	return false;
}

bool CNeedCancel_Manual::CanCancelCover()
{
	if (Get_Manual_Status() == ManualStatus::CancelManul) return true;

	return false;

}
