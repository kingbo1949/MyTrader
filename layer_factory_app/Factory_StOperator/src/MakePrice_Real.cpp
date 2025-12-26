#include "pch.h"
#include "MakePrice_Real.h"
#include <Factory_UnifyInterface.h>
#include <Factory_HashEnv.h>
#include <Factory_Log.h>
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include <Global.h>
#include <Factory_IBJSon.h>
#include "KLineAndMaTool.h"
CMakePrice_Real::CMakePrice_Real(const SubModuleParams& stParams)
	:m_stParams(stParams)
{
	m_pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(m_stParams.strategyIdHashId);
	if (!m_pStrategyParam)
	{
		Log_Print(LogLevel::Err, "Get_StrategyParam return null");
		exit(-1);
	}
	m_targetTickEnv = MakeAndGet_QEnvManager()->GetEnvOneCode(m_stParams.targetCodeId)->GetTicEnv();
	m_targetLastTickEnv = MakeAndGet_QEnvManager()->GetEnvOneCode(m_stParams.targetCodeId)->GetLastTicEnv();
}

bool CMakePrice_Real::GetPrice(double& price, RealPriceType& priceType)
{
	// 预处理，按时限检查是否可以交易
	if (!PreGetPrice())
	{
		//Log_Print(LogLevel::Warn, fmt::format("PreGetPrice fail"));
		return false;
	}

	if (m_targetTickEnv->pTick->bidAsks[0].bid == 0 || m_targetTickEnv->pTick->bidAsks[0].ask == 0)
	{
		Log_Print(LogLevel::Warn, fmt::format("invalid tick"));
		return false;
	}

	if (m_stParams.openOrCover == RoundAction::Open_Round)
	{
		return GetOpenPrice(price, priceType);
	}
	else
	{
		return GetCoverPrice(price, priceType);
	}

}


bool CMakePrice_Real::PreGetPrice()
{
	if (Get_Sys_Status() == Thread_Status::Stop)
	{
		// 系统要求关闭，不允许下单
		return false;
	}

	if (m_stParams.openOrCover == RoundAction::Open_Round && !m_pStrategyParam->canOpen)
	{
		// 配置文件不允许开仓
		return false;
	}
	if (m_stParams.openOrCover == RoundAction::Cover_Round && !m_pStrategyParam->canCover)
	{
		// 配置文件不允许平仓
		return false;
	}


	// 临近收盘是否可以发单
	if (MakeAndGet_TradePointEnv()->NearClose(m_stParams.targetCodeId) && !CanMakePriceNearClose())
	{
		//Log_Print(LogLevel::Err, fmt::format("nearclose return false, {}",
		//	CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond())
		//));
		return false;
	}


	return true;

}

PositionSizePtr CMakePrice_Real::GetOppositPs()
{
	Strategykey key = m_pStrategyParam->key;
	key.buyOrSell = CReverse::Reverse_BuyOrSell(key.buyOrSell);
	PositonSizeKey oppsiteKey = Get_StrategyParamEnv()->Get_StrategyParam_Hash(key.FreshToStr());
	//Log_Print(LogLevel::Info, fmt::format("{}, oppsiteKey = {}", m_pStrategyParam->key.ToStr(), oppsiteKey));

	return MakeAndGet_DbPositionSize()->GetOne(oppsiteKey);


}

OneOrderPtr CMakePrice_Real::GetRecentDealOpenOrder()
{
	OrderKey key;
	key.strategyIdHashId = m_stParams.strategyIdHashId;
	key.stSubModule = StSubModule::SubOpen;
	OneOrderPtrs orders = MakeAndGet_DbInactiveOrder()->GetAll(key);

	OneOrderPtr back = NULL;
	for (auto oneorder : orders)
	{
		if (oneorder->dealedQuantity == 0) continue;
		back = oneorder;
	}
	return back;

}

CodeStr CMakePrice_Real::GetCodeId()
{
	return m_pStrategyParam->key.targetCodeId;
}

Time_Type CMakePrice_Real::GetTimeType()
{
	return m_pStrategyParam->timetype;
}


time_t CMakePrice_Real::Now()
{
	return Get_CurrentTime()->GetCurrentTime_millisecond();
}

time_t CMakePrice_Real::NowM1()
{
	return CGetRecordNo::GetRecordNo(Time_Type::M1, Now());
}

Tick_T CMakePrice_Real::NowM30()
{
	return CGetRecordNo::GetRecordNo(Time_Type::M30, Now());
}

Tick_T CMakePrice_Real::LastM30()
{
	return NowM30() - Tick_T(30) * 60 * 1000;
}


std::string CMakePrice_Real::GetTickTimeStr(time_t ticktime)
{
	return CGlobal::GetTickTimeStr(ticktime);
}


