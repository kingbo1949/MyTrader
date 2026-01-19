#pragma once
#include "../NeedCancel_Real.h"
class CNeedCancel_Manual : 	public CNeedCancel_Real
{
public:
	CNeedCancel_Manual(const SubModuleParams& stParams);
	virtual ~CNeedCancel_Manual() { ; };


protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;

};

