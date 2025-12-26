#pragma once
#include "NeedCancel_Real.h"
class CNeedCancel_On29min : public CNeedCancel_Real
{
public:
	CNeedCancel_On29min(const SubModuleParams& stParams);
	virtual ~CNeedCancel_On29min() { ; };

protected:

	virtual bool			CanCancelOpen() override final;

	virtual bool			CanCancelCover()  override final;


};

