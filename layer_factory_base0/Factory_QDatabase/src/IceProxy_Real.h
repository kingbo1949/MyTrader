#pragma once
#include "IceProxy.h"
class CIceProxy_Real : 	public CIceProxy
{
public:
	CIceProxy_Real();
	virtual ~CIceProxy_Real();

	virtual IBTrader::IQDatabasePrxPtr		GetQDatabasePrx() override final;

	virtual void							ReleaseAllPrx() override final;

protected:

	const std::string				m_iceConfigFileDb;
	Ice::CommunicatorPtr			m_communicatorDb;
	IBTrader::IQDatabasePrxPtr		m_prxQDatabase;
	Ice::CommunicatorPtr			GetCommunicatorDb();



};
 
