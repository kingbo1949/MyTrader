#pragma once
#include "NeedCancel_Real.h"
class CNeedCancel_DoubleBreak :  public CNeedCancel_Real
{
public:
	CNeedCancel_DoubleBreak(const SubModuleParams& stParams);
	virtual ~CNeedCancel_DoubleBreak() { ; };

protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;

};

