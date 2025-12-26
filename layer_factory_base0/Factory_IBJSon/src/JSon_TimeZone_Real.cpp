#include "pch.h"
#include "JSon_TimeZone_Real.h"
#include <Global.h>
#include <Factory_IBGlobalShare.h>

CJSon_TimeZone_Real::CJSon_TimeZone_Real(TimeZone_Type timeZone_Type)
	:m_timeZone_type(timeZone_Type)
{
	m_pTimeZoneOfCodeId = nullptr;
}


void CJSon_TimeZone_Real::Save_TimeZone(TimeZoneOfCodeIdPtr timeZone)
{
	Json::Value timeZoneValue = MakeValue_TimeZone(timeZone);
	SaveJSonValue(GetFileName(), timeZoneValue);

}

TimeZoneOfCodeIdPtr CJSon_TimeZone_Real::Load_TimeZone()
{
	if (!m_pTimeZoneOfCodeId)
	{
		printf("load %s ... \n", GetFileName().c_str());

		Json::Value timeZoneValue;
		LoadJSonValue(GetFileName(), timeZoneValue);

		m_pTimeZoneOfCodeId = Make_TimeZone(timeZoneValue);

	}

	return m_pTimeZoneOfCodeId;
}

std::string CJSon_TimeZone_Real::GetFileName()
{
	if (m_timeZone_type == TimeZone_Type::For_QAnalyst)
	{
		return "./db/QAnalyst_TimeZone.json";
	}
	else
	{
		return "./db/Simulator_TimeZone.json";
	}
}

Json::Value CJSon_TimeZone_Real::MakeValue_TimeZone(TimeZoneOfCodeIdPtr timeZone)
{
	std::string beginPosStr = CGlobal::GetTimeStr(timeZone->beginPos / 1000);
	std::string endPosStr = CGlobal::GetTimeStr(timeZone->endPos / 1000);


	Json::Value back;
	back["codeId"] = timeZone->codeId;
	back["lineType"] = CTransToStr::Get_LineType(timeZone->lineType);
	back["timeType"] = CTransToStr::Get_TimeType(timeZone->timeType);
	back["beginPos"] = beginPosStr;
	back["endPos"] = endPosStr;
	return back;

}

TimeZoneOfCodeIdPtr CJSon_TimeZone_Real::Make_TimeZone(const Json::Value& timeZoneValue)
{
	CodeStr codeId = timeZoneValue["codeId"].asString();
	std::string beginPosStr = timeZoneValue["beginPos"].asString();
	std::string endPosStr = timeZoneValue["endPos"].asString();

	TimeZoneOfCodeIdPtr back = std::make_shared<TimeZoneOfCodeId>();
	back->codeId = codeId;
	back->lineType = CTransToStr::Get_LineType(timeZoneValue["lineType"].asString());
	back->timeType = CTransToStr::Get_TimeType(timeZoneValue["timeType"].asString());
	back->beginPos = CGlobal::GetTimeByStr(beginPosStr) * 1000;
	back->endPos = CGlobal::GetTimeByStr(endPosStr) * 1000;

	return back;

}

