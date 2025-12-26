#pragma once
#include "OnRtn_Real.h"
class COnRtn_Deal : public COnRtn_Real
{
public:
	COnRtn_Deal(const OnTradeMsg& msg);
	virtual ~COnRtn_Deal() { ; };


	virtual void			Execute() override final;

protected:
	OnTradeMsg				m_msg;


	OneDealPtr				MakeDeal();

};

