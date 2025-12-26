#pragma once
#include "JSon_TimeZone.h"
class CJSon_TimeZone_Real : public CJSon_TimeZone
{
public:
	CJSon_TimeZone_Real(TimeZone_Type timeZone_Type) ;
	virtual ~CJSon_TimeZone_Real() { ; };

	virtual void				Save_TimeZone(TimeZoneOfCodeIdPtr timeZone) override final;

	virtual TimeZoneOfCodeIdPtr	Load_TimeZone() override final;

protected:
	TimeZone_Type				m_timeZone_type;
	TimeZoneOfCodeIdPtr			m_pTimeZoneOfCodeId;
	std::string					GetFileName();

	Json::Value					MakeValue_TimeZone(TimeZoneOfCodeIdPtr timeZone);

	TimeZoneOfCodeIdPtr			Make_TimeZone(const Json::Value& timeZoneValue);


};

