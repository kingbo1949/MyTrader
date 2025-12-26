#pragma once
#include "JSon.h"
#include <base_struc.h>
class CJSon_TimeZone : public CJSon
{
public:
	CJSon_TimeZone() { ; };
	virtual ~CJSon_TimeZone() { ; };

	virtual void				Save_TimeZone(TimeZoneOfCodeIdPtr timeZone) = 0;

	virtual TimeZoneOfCodeIdPtr	Load_TimeZone() = 0;


};
typedef std::shared_ptr<CJSon_TimeZone> JSon_TimeZonePtr;


