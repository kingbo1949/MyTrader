#pragma once
#include "Factory_IBGlobalShare_Export.h"

#include <base_trade.h>
class FACTORY_IBGLOBALSHARE_API CReverse
{
public:
	CReverse() { ; };
	virtual ~CReverse() { ; };

	static BuyOrSell			Reverse_BuyOrSell(BuyOrSell buyOrSell);

	static RoundAction			Reverse_OpenOrCover(RoundAction openOrCover);

};

