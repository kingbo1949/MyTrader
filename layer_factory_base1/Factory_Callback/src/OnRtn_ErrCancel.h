#pragma once
#include "OnRtn_Real.h"
class COnRtn_ErrCancel : 	public COnRtn_Real
{
public:
	COnRtn_ErrCancel(OneOrderPtr pOriginalOrder);
	virtual ~COnRtn_ErrCancel() { ; };

	virtual void			Execute() override final;

protected:
	OneOrderPtr				m_pOriginalOrder;

};

