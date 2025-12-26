#pragma once
#include "JSon.h"
#include <base_trade.h>
class CJSon_Order : public CJSon
{
public:
	CJSon_Order() { ; };
	virtual ~CJSon_Order() { ; };

	virtual void			Save_Orders(const OneOrderPtrs& orders) = 0;

	virtual void			Load_Orders(OneOrderPtrs& orders) = 0;

};
typedef std::shared_ptr<CJSon_Order> JSon_OrderPtr;

