#pragma once
#include <Env.h>
#include "DbTable_TickHis.h"
#include "DbTable_KLine.h"
#include "DbTable_Average.h"
#include "DbTable_DivType.h"
#include "DbTable_Macd.h"
// IB的行情数据库环境
class CEnv_IB : public CEnv
{
public:
	CEnv_IB(const std::string& path, const std::string& dir, int sizeG, int sizeM, u_int32_t mutexInit, u_int32_t mutexIncrement);
	virtual ~CEnv_IB(void);

	DbTable_TickHisPtr						GetDB_TickHis();

	DbTable_KLinePtr						GetDB_KLine();

	DbTable_AveragePtr						GetDB_Ma();

	DbTable_AveragePtr						GetDB_VwMa();

	DbTable_AveragePtr						GetDB_Ema();

	DbTable_MacdPtr							GetDB_Macd();

	DbTable_DivTypePtr						GetDB_DivType();

	void									OpenAllTable();
};
typedef std::shared_ptr<CEnv_IB> Env_IBPtr;

