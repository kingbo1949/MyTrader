#pragma once

#ifdef WIN32

#ifdef FACTORY_IBGLOBALSHARE_EXPORTS
#define FACTORY_IBGLOBALSHARE_API __declspec(dllexport)
#else
#define FACTORY_IBGLOBALSHARE_API __declspec(dllimport)
#endif


#else

#define FACTORY_IBGLOBALSHARE_API 

#endif // WIN32

#include <base_trade.h>
class FACTORY_IBGLOBALSHARE_API CReverse
{
public:
	CReverse() { ; };
	virtual ~CReverse() { ; };

	static BuyOrSell			Reverse_BuyOrSell(BuyOrSell buyOrSell);

	static RoundAction			Reverse_OpenOrCover(RoundAction openOrCover);

};

