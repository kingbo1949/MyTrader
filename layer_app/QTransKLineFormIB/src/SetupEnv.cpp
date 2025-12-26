#include "SetupEnv.h"

#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_QDatabase.h>
#include <Factory_UnifyInterface.h>
#include <Global.h>

#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;


void CSetupEnv::Init()
{
	Init_Factory_Log();
	Init_Factory_Global();
	Init_Factory_CodeIdHashEnv();
	Init_Factory_QDatabase();

	return;

}

void CSetupEnv::RunIBApi()
{
	Make_IBApi(SetupType::QGenerator);
	Get_IBApi()->Init();
	Get_IBApi()->Connect();
	return;

}

void CSetupEnv::Release()
{
	// 关闭远程api，同时也会导致各种守护线程关闭
	Get_IBApi()->Release();

	// 关闭行情数据连接
	ReleaseQDatabase();

	//std::this_thread::sleep_for(std::chrono::seconds(2));

	// 关闭日志系统
	Log_Close();

	return;

}

void CSetupEnv::Init_Factory_Log()
{
	Make_Log();
	return;

}

void CSetupEnv::Init_Factory_Global()
{
	MakeAndGet_MyThreadPool();
	MakeAndGet_SeriesNoMaker();
	Make_CurrentTime(CurrentTimeType::For_Real, 0);

	return;

}

void CSetupEnv::Init_Factory_CodeIdHashEnv()
{
	// 代码哈希属性表
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	Make_CodeIdEnv(CTrans::Get_CodeIds(contracts));
	return;

}

void CSetupEnv::Init_Factory_QDatabase()
{
	// 与行情数据库的连接和ice
	MakeAndGet_QDatabase();

	// 交易时点
	MakeAndGet_TradePointEnv();

	// 代码属性
	MakeAndGet_ContractEnv();

	return;

}
