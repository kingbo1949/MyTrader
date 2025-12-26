#pragma once
#include "JSon.h"
#include <base_trade.h>
class CJSon_Deal : public CJSon
{
public:
	CJSon_Deal() { ; };
	virtual ~CJSon_Deal() { ; };

	virtual void			Save_Deals(const OneDealPtrs& deals) = 0;

	virtual void			Load_Deals(OneDealPtrs& deals) = 0;

};
typedef std::shared_ptr<CJSon_Deal> JSon_DealPtr;


