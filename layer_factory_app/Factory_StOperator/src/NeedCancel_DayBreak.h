#pragma once
#include "NeedCancel_Real.h"
class CNeedCancel_DayBreak : public CNeedCancel_Real
{
public:
	CNeedCancel_DayBreak(const SubModuleParams& stParams);
	virtual ~CNeedCancel_DayBreak() { ; };

protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;

};

