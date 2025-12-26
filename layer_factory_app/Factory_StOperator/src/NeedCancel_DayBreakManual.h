#pragma once
#include "NeedCancel_Real.h"
class CNeedCancel_DayBreakManual : public CNeedCancel_Real
{
public:
	CNeedCancel_DayBreakManual(const SubModuleParams& stParams);
	virtual ~CNeedCancel_DayBreakManual() { ; };

protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;

};

