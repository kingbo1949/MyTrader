#include "pch.h"
#include "IceProxy_Real.h"
#include <Factory_Log.h>

CIceProxy_Real::CIceProxy_Real()
	:m_iceConfigFileDb("config.server")
{

}

CIceProxy_Real::~CIceProxy_Real()
{

}

IBTrader::IQDatabasePrxPtr CIceProxy_Real::GetQDatabasePrx()
{
	if (!m_prxQDatabase)
	{
		std::string proxyStr = "";
		try
		{
			proxyStr = GetCommunicatorDb()->getProperties()->getProperty("IBQDatabase.Proxy");
			Ice::ObjectPrxPtr obj = GetCommunicatorDb()->stringToProxy(proxyStr);
			m_prxQDatabase = Ice::checkedCast<IBTrader::IQDatabasePrx>(obj);
			//m_prxQDatabase = IQDatabasePrx::checkedCast(obj);
			//m_prxQDatabase = IDatabasePrx::uncheckedCast(obj);
		}
		catch (const Ice::Exception& ex)
		{
			printf("%s %s \n", ex.ice_id().c_str(), ex.what());
			Log_Print(LogLevel::Err, fmt::format("Cant connect to QDatabase:{} ", proxyStr.c_str()));
			exit(1);
		}

	}
	return m_prxQDatabase;

}

void CIceProxy_Real::ReleaseAllPrx()
{
	if (m_communicatorDb)
	{
		m_communicatorDb->shutdown();
		m_communicatorDb->destroy();
		m_communicatorDb = nullptr;
		Log_Print(LogLevel::Info, "m_communicatorDb destroy \n");
	}

	return;
}

std::shared_ptr<Ice::Communicator> CIceProxy_Real::GetCommunicatorDb()
{
	if (!m_communicatorDb)
	{
		// 从配置文件初始化属性
		Ice::PropertiesPtr properties = Ice::createProperties();
		properties->load(m_iceConfigFileDb);

		Ice::InitializationData initData;
		initData.properties = properties;

		m_communicatorDb = Ice::initialize(initData);
	}
	return m_communicatorDb;

}
