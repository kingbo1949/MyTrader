#pragma once
#include "OnRtn_Real.h"
class COnRtn_Order : public COnRtn_Real
{
public:
	COnRtn_Order(const OnOrderMsg& msg);
	virtual ~COnRtn_Order() { ; };

	virtual void			Execute() override ;

protected:
	OnOrderMsg				m_msg;
	double					m_newDealQuantity = 0.0;		// 与数据库中的老数据对比，新成交的手数



};

