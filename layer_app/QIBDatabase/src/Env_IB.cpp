#include "Env_IB.h"

CEnv_IB::CEnv_IB(const std::string& path, const std::string& dir, int sizeG, int sizeM, u_int32_t mutexInit, u_int32_t mutexIncrement)
	:CEnv(path, dir, sizeG, sizeM, mutexInit, mutexIncrement)
{

}

CEnv_IB::~CEnv_IB(void)
{

}

Bdb::DbTable_TickHisPtr CEnv_IB::GetDB_TickHis()
{
	const std::string dbName = "tick_his.db";

	DbTablePtr pDb = GetDbFile(dbName);
	if (!pDb)
	{
		pDb = new CDbTable_TickHis(&m_Env, m_path + m_dir + "\\", dbName);
		AddDbFile(pDb);
	}
	DbTable_TickHisPtr pBack = DbTable_TickHisPtr::dynamicCast(pDb);
	return pBack;


}

Bdb::DbTable_KLinePtr CEnv_IB::GetDB_KLine()
{
	const std::string dbName = "kline_his.db";

	DbTablePtr pDb = GetDbFile(dbName);
	if (!pDb)
	{
		pDb = new CDbTable_KLine(&m_Env, m_path + m_dir + "\\", dbName);
		AddDbFile(pDb);
	}
	DbTable_KLinePtr pBack = DbTable_KLinePtr::dynamicCast(pDb);
	return pBack;

}

Bdb::DbTable_AveragePtr CEnv_IB::GetDB_Ma()
{
	const std::string dbName = "ma_his.db";

	DbTablePtr pDb = GetDbFile(dbName);
	if (!pDb)
	{
		pDb = new CDbTable_Average(&m_Env, m_path + m_dir + "\\", dbName);
		AddDbFile(pDb);
	}
	DbTable_AveragePtr pBack = DbTable_AveragePtr::dynamicCast(pDb);
	return pBack;


}

DbTable_AveragePtr CEnv_IB::GetDB_VwMa()
{
	const std::string dbName = "vwma_his.db";

	DbTablePtr pDb = GetDbFile(dbName);
	if (!pDb)
	{
		pDb = new CDbTable_Average(&m_Env, m_path + m_dir + "\\", dbName);
		AddDbFile(pDb);
	}
	DbTable_AveragePtr pBack = DbTable_AveragePtr::dynamicCast(pDb);
	return pBack;
}

DbTable_AveragePtr CEnv_IB::GetDB_Ema()
{
	const std::string dbName = "ema_his.db";

	DbTablePtr pDb = GetDbFile(dbName);
	if (!pDb)
	{
		pDb = new CDbTable_Average(&m_Env, m_path + m_dir + "\\", dbName);
		AddDbFile(pDb);
	}
	DbTable_AveragePtr pBack = DbTable_AveragePtr::dynamicCast(pDb);
	return pBack;
}

DbTable_MacdPtr CEnv_IB::GetDB_Macd()
{
	const std::string dbName = "macd_his.db";

	DbTablePtr pDb = GetDbFile(dbName);
	if (!pDb)
	{
		pDb = new CDbTable_Macd(&m_Env, m_path + m_dir + "\\", dbName);
		AddDbFile(pDb);
	}
	DbTable_MacdPtr pBack = DbTable_MacdPtr::dynamicCast(pDb);
	return pBack;
}

DbTable_DivTypePtr CEnv_IB::GetDB_DivType()
{
	const std::string dbName = "divtype_his.db";

	DbTablePtr pDb = GetDbFile(dbName);
	if (!pDb)
	{
		pDb = new CDbTable_DivType(&m_Env, m_path + m_dir + "\\", dbName);
		AddDbFile(pDb);
	}
	DbTable_DivTypePtr pBack = DbTable_DivTypePtr::dynamicCast(pDb);
	return pBack;
}

void CEnv_IB::OpenAllTable()
{
	GetDB_TickHis();
	GetDB_KLine();
	GetDB_Ma();
	GetDB_VwMa(); 
	GetDB_Ema();
	GetDB_Macd();
	GetDB_DivType();


	return;
}
