#include "pch.h"
#include "JSon_Broker_Real.h"
#include <TransToStr.h>

void CJSon_Broker_Real::SaveBroker(const OneBrokerPtr& broker)
{
	Json::Value brokerValue = MakeValue_Broker(broker);
	SaveJSonValue(GetFileName(), brokerValue);
}

OneBrokerPtr CJSon_Broker_Real::LoadBroker()
{
	if (!m_pBroker)
	{
		printf("load %s ... \n", GetFileName().c_str());
		Json::Value brokerValue;
		LoadJSonValue(GetFileName(), brokerValue);

		m_pBroker = Make_Broker(brokerValue);

	}

	return m_pBroker;
}

std::string CJSon_Broker_Real::GetFileName()
{
	return "./db/broker.json";
}

Json::Value CJSon_Broker_Real::MakeValue_Broker(const OneBrokerPtr& broker)
{
	Json::Value back;

	back["gatewayIp"] = broker->gatewayIp;
	back["gatewayPort"] = broker->gatewayPort;
	back["connId"] = broker->connId;

	std::string str = CTransToStr::Get_SetupType(broker->setupType);
	back["setupType"] = str;

	return back;


}

OneBrokerPtr CJSon_Broker_Real::Make_Broker(const Json::Value& brokerValue)
{
	OneBrokerPtr back = std::make_shared<COneBroker>();

	back->gatewayIp = brokerValue["gatewayIp"].asString();
	back->gatewayPort = brokerValue["gatewayPort"].asInt();;
	back->connId = brokerValue["connId"].asInt();
	back->setupType = CTransToStr::Get_SetupType(brokerValue["setupType"].asString() );

	return back;

}
