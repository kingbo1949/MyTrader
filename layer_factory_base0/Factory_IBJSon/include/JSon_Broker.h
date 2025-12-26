#pragma once
#include "JSon.h"
#include <base_struc.h>
class CJSon_Broker : public CJSon
{
public:
	CJSon_Broker() { ; };
	virtual ~CJSon_Broker() { ; };

	virtual		void					SaveBroker(const OneBrokerPtr& broker) = 0;

	virtual		OneBrokerPtr			LoadBroker() = 0;

};
typedef std::shared_ptr<CJSon_Broker> JSon_BrokerPtr;


