#pragma once
#include "JSon_Broker.h"
class CJSon_Broker_Real : public CJSon_Broker
{
public:
	CJSon_Broker_Real():m_pBroker(nullptr) { ; };
	virtual ~CJSon_Broker_Real() { ; };


	virtual		void					SaveBroker(const OneBrokerPtr& broker);

	virtual		OneBrokerPtr			LoadBroker();


protected:
	OneBrokerPtr						m_pBroker;

	std::string							GetFileName();

	Json::Value							MakeValue_Broker(const OneBrokerPtr& broker);

	OneBrokerPtr						Make_Broker(const Json::Value& brokerValue);



};

