#include "Env_IB.h"

CEnv_IB::CEnv_IB(const std::string& dbPath)
	: m_env(dbPath)
{
}

CEnv_IB::~CEnv_IB()
{
}

void CEnv_IB::OpenAllTable()
{
	m_tickHis = std::make_unique<CDbTable_TickHis>(m_env, "TICK");
	m_kline   = std::make_unique<CDbTable_KLine>(m_env, "KLINE");
	m_ma      = std::make_unique<CDbTable_Average>(m_env, "MA");
	m_vwma    = std::make_unique<CDbTable_Average>(m_env, "VWMA");
	m_ema     = std::make_unique<CDbTable_Average>(m_env, "EMA");
	m_macd    = std::make_unique<CDbTable_Macd>(m_env, "MACD");
	m_divType = std::make_unique<CDbTable_DivType>(m_env, "DIVTYPE");
	m_atr     = std::make_unique<CDbTable_Atr>(m_env, "ATR");

	printf("All RocksDB tables opened\n");
}

void CEnv_IB::Flush()
{
	m_env.Flush();
}
