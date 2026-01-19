#pragma once
#include "../NeedCancel_Real.h"

class CNeedCancel_UTurn : public CNeedCancel_Real
{
public:
	explicit CNeedCancel_UTurn(const SubModuleParams& stParams);
	~CNeedCancel_UTurn() override { ; };


protected:

	bool			CanCancelOpen() final;

	bool			CanCancelCover() final;

};


