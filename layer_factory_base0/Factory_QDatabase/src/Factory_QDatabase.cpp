#include "pch.h"
#include "Factory_QDatabase.h"
#include "ContractEnv_Real.h"
#include "TradePointEnv_Real.h"
#include "MyQDatabase_Real.h"
#include "Factory_IceProxy.h"
#include <Factory_Log.h>
#include "WhiteBoard_Real.h"
#include "KLineBoard_Real.h"
#include "TickBoard_Real.h"
#include <Global.h>
#include <Factory_HashEnv.h>
#include "CodeIdManager_Real.h"
#include "MarketDepth_Real.h"
#include "CheckTrend_Price.h"
#include "CheckTrend_Crose.h"



MyQDatabasePtr m_pQDatabase = nullptr;

FACTORY_QDATABASE_API MyQDatabasePtr MakeAndGet_QDatabase()
{
	if (!m_pQDatabase)
	{
		m_pQDatabase = std::make_shared<CMyQDatabase_Real>();

		// 测试代理
		MakeAndGet_IceProxy()->GetQDatabasePrx();

	}
	return m_pQDatabase;
}

FACTORY_QDATABASE_API void ReleaseQDatabase()
{
	MakeAndGet_IceProxy()->ReleaseAllPrx();
	return;

}

CodeIdManagerPtr g_codeIdManager = nullptr;
FACTORY_QDATABASE_API void Make_CodeIdManager(SetupType setupType)
{
	if (!g_codeIdManager)
	{
		g_codeIdManager = std::make_shared<CCodeIdManager_Real>(setupType);
	}
}

FACTORY_QDATABASE_API CodeIdManagerPtr Get_CodeIdManager()
{
	if (!g_codeIdManager)
	{
		printf("g_codeIdManager is null \n");
		exit(-1);
	}
	return g_codeIdManager;

}

WhiteBoardPtr g_whitboard = nullptr;
FACTORY_QDATABASE_API WhiteBoardPtr MakeAndGet_WhiteBoard()
{
	if (!g_whitboard)
	{
		g_whitboard = std::make_shared<CWhiteBoard_Real>();
	}
	return g_whitboard;
}

MarketDepthPtr g_marketDepth = nullptr;
FACTORY_QDATABASE_API MarketDepthPtr MakeAndGet_MarketDepth()
{
	if (!g_marketDepth)
	{
		g_marketDepth = std::make_shared<CMarketDepth_Real>();
	}
	return g_marketDepth;
}

KLineBoardPtr g_klineboard = nullptr;
FACTORY_QDATABASE_API KLineBoardPtr MakeAndGet_KLineBoard()
{
	if (!g_klineboard)
	{
		g_klineboard = std::make_shared<CKLineBoard_Real>();
	}
	return g_klineboard;
}

TickBoardPtr g_tickboard = nullptr;
FACTORY_QDATABASE_API TickBoardPtr MakeAndGet_TickBoard()
{
	if (!g_tickboard)
	{
		g_tickboard = std::make_shared<CTickBoard_Real>();
	}
	return g_tickboard;
}

ContractEnvPtr g_contractEnv = nullptr;
FACTORY_QDATABASE_API ContractEnvPtr MakeAndGet_ContractEnv()
{
	if (!g_contractEnv)
	{
		g_contractEnv = std::make_shared<CContractEnv_Real>();
	}
	return g_contractEnv;
}

TradePointEnvPtr g_timePointEnv = nullptr;
FACTORY_QDATABASE_API TradePointEnvPtr MakeAndGet_TradePointEnv()
{
	if (!g_timePointEnv)
	{
		g_timePointEnv = std::make_shared<CTradePointEnv_Real>();
	}
	return g_timePointEnv;
}

FACTORY_QDATABASE_API CheckTrendPtr MakeAndGet_CheckTrend(const CodeStr& codeId, Time_Type timeType, int circle, Tick_T currentTime)
{
	CheckTrendPtr ret = std::make_shared<CCheckTrend_Price>(codeId, timeType, circle, currentTime);
	return ret;
}

FACTORY_QDATABASE_API CheckTrendPtr MakeAndGet_CheckTrend(const CodeStr& codeId, Time_Type timeType, int circleFast, int circleSlow, Tick_T currentTime)
{
	CheckTrendPtr ret = std::make_shared<CCheckTrend_Crose>(codeId, timeType, circleFast, circleSlow, currentTime);
	return ret;
}

