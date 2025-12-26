#pragma once
#include "NeedCancel_Real.h"
class CNeedCancel_BreakInDay : public CNeedCancel_Real
{
public:
	CNeedCancel_BreakInDay(const SubModuleParams& stParams);
	virtual ~CNeedCancel_BreakInDay() { ; };


protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;

};

