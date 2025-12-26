#include "pch.h"
#include "db_PositionSize_Real.h"
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Global.h>
FileStatus Cdb_PositionSize_Real::GetStatus()
{
	return m_status;
}

void Cdb_PositionSize_Real::SetStatus(FileStatus fileStatus)
{
	m_status = fileStatus;
}


void Cdb_PositionSize_Real::NewDeal(const OnTradeMsg& tradeMsg)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_status = FileStatus::Updated;

	StrategyIdHashId strategyId = tradeMsg.pOriginalOrder->orderKey.strategyIdHashId;

	if (tradeMsg.pOriginalOrder->openOrCover == RoundAction::Open_Round)
	{
		// 开仓
		std::map<PositonSizeKey, PositionSizePtr>::iterator pos = m_positionSizes.find(strategyId);
		if (pos != m_positionSizes.end())
		{
			PositionSizePtr ps = pos->second;
			ps->avgPrice = (ps->vol * ps->avgPrice + tradeMsg.dealQuantity * tradeMsg.dealPrice) / (ps->vol + tradeMsg.dealQuantity);
			ps->vol += tradeMsg.dealQuantity;

		}
		else
		{
			PositionSizePtr ps = std::make_shared<CPositionSize>();
			ps->key = strategyId;
			ps->codeHashId = Get_CodeIdEnv()->Get_CodeId_Hash(Get_StrategyParamEnv()->Get_StrategyParam(strategyId)->key.targetCodeId.c_str());
			ps->openTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();
			ps->vol = tradeMsg.dealQuantity;;
			ps->avgPrice = tradeMsg.dealPrice;

			if (tradeMsg.pOriginalOrder->buyOrSell == BuyOrSell::Buy)
			{
				ps->longOrShort = LongOrShort::LongT;
			}
			else
			{
				ps->longOrShort = LongOrShort::ShortT;
			}
			m_positionSizes[strategyId] = ps;

		}
	}
	else
	{
		// 平仓
		std::map<PositonSizeKey, PositionSizePtr>::iterator pos = m_positionSizes.find(strategyId);
		if (pos == m_positionSizes.end())
		{
			// 平仓时候找不到持仓
			Log_Print(LogLevel::Err, fmt::format("want to cover, but there is no position, {} cover {}, vol = {}",
				Get_StrategyParamEnv()->Get_StrategyParam(strategyId)->key.ToStr(), 
				CTransToStr::Get_BuyOrSell(tradeMsg.pOriginalOrder->buyOrSell).c_str(), 
				tradeMsg.dealQuantity
			));
			return;
		}
		if (CGlobal::DToI(pos->second->vol)  < CGlobal::DToI(tradeMsg.dealQuantity) )
		{
			// 可平持仓太少
			Log_Print(LogLevel::Err, fmt::format("want to cover, but the available position is not enough, {} cover {}, vol = {}, exist ps = {}",
				Get_StrategyParamEnv()->Get_StrategyParam(strategyId)->key.ToStr(), 
				CTransToStr::Get_BuyOrSell(tradeMsg.pOriginalOrder->buyOrSell).c_str(), tradeMsg.dealQuantity, pos->second->vol
			));
			exit(-1);

		}
		pos->second->vol -= tradeMsg.dealQuantity;
		if (CGlobal::IsZero(pos->second->vol))
		{
			m_positionSizes.erase(pos);
		}
	}



}

void Cdb_PositionSize_Real::AddOne(PositionSizePtr positionSize)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_status = FileStatus::Updated;

	m_positionSizes[positionSize->key] = positionSize;
	return;
}

void Cdb_PositionSize_Real::RemoveOne(const PositonSizeKey& key)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_status = FileStatus::Updated;

	std::map<PositonSizeKey, PositionSizePtr>::iterator pos = m_positionSizes.find(key);
	if (pos == m_positionSizes.end()) return;

	m_positionSizes.erase(pos);
	return;

}

PositionSizePtr Cdb_PositionSize_Real::GetOne(const PositonSizeKey& key)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::map<PositonSizeKey, PositionSizePtr>::iterator pos = m_positionSizes.find(key);
	if (pos == m_positionSizes.end()) return nullptr;

	return pos->second;

}

PositionSizePtrs Cdb_PositionSize_Real::GetAll()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	PositionSizePtrs back;
	for (auto oneitem : m_positionSizes)
	{
		back.push_back(oneitem.second);
	}
	return back;
}
