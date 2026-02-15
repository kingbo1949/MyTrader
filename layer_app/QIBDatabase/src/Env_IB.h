#pragma once
#include <RocksEnv.h>
#include "./DbTable/DbTable_TickHis.h"
#include "./DbTable/DbTable_KLine.h"
#include "./DbTable/DbTable_Average.h"
#include "./DbTable/DbTable_DivType.h"
#include "./DbTable/DbTable_Macd.h"
#include "./DbTable/CDbTable_Atr.h"

// IB的行情数据库环境（RocksDB 版本）
class CEnv_IB
{
public:
	explicit CEnv_IB(const std::string& dbPath);
	~CEnv_IB();

	CRocksEnv&		GetEnv() { return m_env; }

	CDbTable_TickHis*	GetDB_TickHis()  { return m_tickHis.get(); }
	CDbTable_KLine*		GetDB_KLine()    { return m_kline.get(); }
	CDbTable_Average*	GetDB_Ma()       { return m_ma.get(); }
	CDbTable_Average*	GetDB_VwMa()     { return m_vwma.get(); }
	CDbTable_Average*	GetDB_Ema()      { return m_ema.get(); }
	CDbTable_Macd*		GetDB_Macd()     { return m_macd.get(); }
	CDbTable_DivType*	GetDB_DivType()  { return m_divType.get(); }
	CDbTable_Atr*		GetDB_Atr()      { return m_atr.get(); }

	void			OpenAllTable();
	void			Flush();

private:
	CRocksEnv m_env;

	std::unique_ptr<CDbTable_TickHis>	m_tickHis;
	std::unique_ptr<CDbTable_KLine>		m_kline;
	std::unique_ptr<CDbTable_Average>	m_ma;
	std::unique_ptr<CDbTable_Average>	m_vwma;
	std::unique_ptr<CDbTable_Average>	m_ema;
	std::unique_ptr<CDbTable_Macd>		m_macd;
	std::unique_ptr<CDbTable_DivType>	m_divType;
	std::unique_ptr<CDbTable_Atr>		m_atr;
};
typedef std::shared_ptr<CEnv_IB> Env_IBPtr;
