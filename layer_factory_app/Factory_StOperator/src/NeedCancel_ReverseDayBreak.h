#pragma once
#include "NeedCancel_Real.h"
class CNeedCancel_ReverseDayBreak :  public CNeedCancel_Real
{
public:
	CNeedCancel_ReverseDayBreak(const SubModuleParams& stParams);
	virtual ~CNeedCancel_ReverseDayBreak() { ; };

protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;

};

