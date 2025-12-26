#pragma once
#include "JSon_Order.h"
class CJSon_Order_Real : public CJSon_Order
{
public:
	CJSon_Order_Real(ActiveOrInactive activeOrInactive);
	virtual ~CJSon_Order_Real() { ; };

	virtual void			Save_Orders(const OneOrderPtrs& orders) override final;

	virtual void			Load_Orders(OneOrderPtrs& orders) override final;


protected:
	ActiveOrInactive		m_activeOrInactive;
	std::string				GetFileName();

	Json::Value				MakeValue_OrderKey(const OrderKey& orderkey);
	OrderKey				Make_OrderKey(const Json::Value& orderkeyValue);


	Json::Value				MakeValue_Order(OneOrderPtr& oneorder);
	OneOrderPtr				Make_Order(const Json::Value& oneorderValue);

	Json::Value				MakeValue_Orders(const OneOrderPtrs& orders);
	OneOrderPtrs			Make_Orders(const Json::Value& ordersValue);

};

