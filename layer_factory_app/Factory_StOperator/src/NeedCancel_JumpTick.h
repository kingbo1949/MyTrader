#pragma once
#include "NeedCancel_Real.h"
class CNeedCancel_JumpTick :  public CNeedCancel_Real
{
public:
	CNeedCancel_JumpTick(const SubModuleParams& stParams);
	virtual ~CNeedCancel_JumpTick() { ; };

protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;

};

