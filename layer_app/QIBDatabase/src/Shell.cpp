#include "pch.h"
#include "Shell.h"
#include "QDatabaseImp.h"
#include "Factory.h"
#include <Factory_Log.h>
CShell::CShell(void)
{

}

CShell::~CShell(void)
{

}

int CShell::run(int, char* [])
{
	callbackOnInterrupt();

	SetCommunicator(communicator());
	Make_Log();


	SetupEnv();

	// 添加接口
	Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("Adapter_IBQDatabase");
	QDatabaseImpPtr pServer = std::make_shared<CQDatabaseImp>();

	Identity identity;
	identity.category = "QServer";
	identity.name = "IBQDatabase";
	ObjectPrxPtr prx = adapter->add(pServer, identity);

	//启动接口之后再开始定时线程
	try
	{
		adapter->activate();
		printf("adapter activate \n");
	}
	catch (IceUtil::Exception& e)
	{
		printf("\n.....adapter activate err.....\n\n%s\n\n", e.what());
		exit(-1);
	}
	MakeAndGet_MyThreadPool();

	m_pTimerJob_UpdateIndex = new Timer();
	m_pTimerJob_UpdateIndex->scheduleRepeated(MakeAndGet_TimerTask_UpdateIndex(), Time::milliSeconds(10000));

	communicator()->waitForShutdown();

	printf("will EXIT_SUCCESS \n");
	return EXIT_SUCCESS;

}

void CShell::interruptCallback(int signal)
{
	printf("Will Exit: \n");

	while (!MakeAndGet_MyThreadPool()->isAllIdle())
	{
		printf("Idle thread count %d \n", MakeAndGet_MyThreadPool()->idlCount());
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	if (m_pTimerJob_UpdateIndex)
	{
		m_pTimerJob_UpdateIndex->destroy();
	}
	printf("m_pTimerJob_UpdateIndex destroy\n");

	MakeAndGet_Env()->Flush();
	printf("RocksDB flushed\n");

	communicator()->shutdown();

}

void CShell::SetupEnv()
{
	// 启动Env
	MakeAndGet_Env()->OpenAllTable();
}

