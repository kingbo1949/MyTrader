#pragma once
#include "Factory_QDatabase_Export.h"


// 模块功能：
// 1、提供ice行情数据库的访问接口

#include "ContractEnv.h"
#include "MyQDatabase.h"
#include "WhiteBoard.h"
#include "KLineBoard.h"
#include "TickBoard.h"
#include "TradePointEnv.h"
#include "CodeIdManager.h"
#include "MarketDepth.h"
#include "CheckTrend.h"


	FACTORY_QDATABASE_API MyQDatabasePtr			MakeAndGet_QDatabase();
	FACTORY_QDATABASE_API void						ReleaseQDatabase();

	FACTORY_QDATABASE_API void						Make_CodeIdManager(SetupType setupType);
	FACTORY_QDATABASE_API CodeIdManagerPtr			Get_CodeIdManager();


	FACTORY_QDATABASE_API WhiteBoardPtr				MakeAndGet_WhiteBoard();

	FACTORY_QDATABASE_API MarketDepthPtr			MakeAndGet_MarketDepth();

	FACTORY_QDATABASE_API KLineBoardPtr				MakeAndGet_KLineBoard();

	FACTORY_QDATABASE_API TickBoardPtr				MakeAndGet_TickBoard();

	FACTORY_QDATABASE_API ContractEnvPtr			MakeAndGet_ContractEnv();

	FACTORY_QDATABASE_API TradePointEnvPtr			MakeAndGet_TradePointEnv();

	FACTORY_QDATABASE_API CheckTrendPtr 			MakeAndGet_CheckTrend(const CodeStr& codeId, Time_Type timeType, int circle, Tick_T currentTime);
	FACTORY_QDATABASE_API CheckTrendPtr 			MakeAndGet_CheckTrend(const CodeStr& codeId, Time_Type timeType, int circleFast, int circleSlow, Tick_T currentTime);





