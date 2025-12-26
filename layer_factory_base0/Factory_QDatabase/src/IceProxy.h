#pragma once

#include <memory>
#include <string>
#include <Ice/Ice.h>

#include <QDatabase.h>


//using namespace Ice;
//using namespace IBTrader;

class CIceProxy
{
public:
	CIceProxy() { ; };
	virtual ~CIceProxy() { ; };

	virtual IBTrader::IQDatabasePrxPtr		GetQDatabasePrx() = 0;

	virtual void							ReleaseAllPrx() = 0;
};

typedef std::shared_ptr<CIceProxy> IceProxyPtr;
