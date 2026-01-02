#include "TickUpdator.h"
#include "Factory.h"
#include "GetRecordNoForDb.h"
#include "Calculator.h"
#include <Factory_Log.h>
CTickUpdator::CTickUpdator()
{
}

void CTickUpdator::UpdateTickToDB(const ITick& tick)
{
	//UpdateTickToTickHis(tick);
	UpdateTickToKLine(tick);

}

void CTickUpdator::UpdateTickToTickHis(const ITick& tick)
{
	MakeAndGet_Env()->GetDB_TickHis()->AddOne(tick);

	return;
}

void CTickUpdator::UpdateTickToKLine(const ITick& tick)
{
	if (tick.vol == 0)
	{
		printf("%s drop tick\n", tick.codeId.c_str());
		return;
	}
	if (GetSecurityType(tick.codeId) == SecurityType::FUT)
	{
		// 只有期货需要15秒线
		//UpdateTickToKLine(tick, ITimeType::S15);
	}
	UpdateTickToKLine(tick, ITimeType::M1);
	UpdateTickToKLine(tick, ITimeType::M5);
	UpdateTickToKLine(tick, ITimeType::M15);
	UpdateTickToKLine(tick, ITimeType::M30);
	UpdateTickToKLine(tick, ITimeType::H1);
	UpdateTickToKLine(tick, ITimeType::D1);
	return;

}


void CTickUpdator::UpdateTickToKLine(const ITick& tick, ITimeType timeType)
{
	time_t	recordNo = CGetRecordNoForDb::GetRecordNo(timeType, tick.time);

	IKLine kline;
	bool exist = MakeAndGet_Env()->GetDB_KLine()->GetOne(tick.codeId, timeType, recordNo, kline);
	if (exist)
	{
		kline.close = tick.last;
		if (tick.last > kline.high)
		{
			kline.high = tick.last;
		}
		if (tick.last < kline.low)
		{
			kline.low = tick.last;
		}
		kline.vol += tick.vol;
	}
	else
	{
		kline.time = recordNo;
		kline.open = tick.last;
		kline.close = tick.last;
		kline.high = tick.last;
		kline.low = tick.last;
		kline.vol = tick.vol;
	}
	if (!ValidKline(tick.codeId, timeType, kline))
	{
		return;
	}

	MakeAndGet_Env()->GetDB_KLine()->AddOne(tick.codeId, timeType, kline);

	if (!CanUpdateIndex(tick.codeId, timeType) && exist)
	{
		// K线已经存在但不久前扫描过，不要更新
		return;
	}

	// 需要扫描并更新指标
	if (!exist)
	{
		// 新生成的K线
		MakeAndGet_TimerTask_UpdateIndex()->AddNeedUpdate(tick.codeId, timeType, NowOrWait::Now);
	}
	{
		// 旧K线
		MakeAndGet_TimerTask_UpdateIndex()->AddNeedUpdate(tick.codeId, timeType, NowOrWait::Wait);

	}
	ChgLastUpdateIndexTime(tick.codeId, timeType);



	return;
}

bool CTickUpdator::CanUpdateIndex(const std::string& codeId, ITimeType timeType)
{
	time_t timeSpec = GetTimeSpec(codeId, timeType);

	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;
	auto pos = m_lastUpdateTime.find(key);
	if (pos == m_lastUpdateTime.end())
	{
		return true;
	}
	else
	{
		if (benchmark_milliseconds() - pos->second < timeSpec)
		{
			// 距离上次更新时间太短
			return false;
		}
		else
		{
			return true;
		}

	}
}

void CTickUpdator::ChgLastUpdateIndexTime(const std::string& codeId, ITimeType timeType)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;
	m_lastUpdateTime[key] = benchmark_milliseconds();
	return;

}

time_t CTickUpdator::GetTimeSpec(const std::string& codeId, ITimeType timeType)
{
	return 10000;


	//if (GetSecurityType(codeId) == SecurityType::FUT)
	//{
	//	// 期货
	//	switch (timeType)
	//	{
	//	case IBTrader::ITimeType::S15:
	//		return 1000;
	//		break;
	//	case IBTrader::ITimeType::M1:
	//		return 1500;
	//		break;
	//	case IBTrader::ITimeType::M5:
	//		return 2000;
	//		break;
	//	case IBTrader::ITimeType::M15:
	//		return 2000;
	//		break;
	//	case IBTrader::ITimeType::M30:
	//		return 3000;
	//		break;
	//	case IBTrader::ITimeType::H1:
	//		return 5000;
	//		break;
	//	case IBTrader::ITimeType::D1:
	//		return 5000;
	//		break;
	//	default:
	//		break;
	//	}
	//}
	//else 
	//{
	//	// 股票
	//	switch (timeType)
	//	{
	//	case IBTrader::ITimeType::S15:
	//		return 5000;
	//		break;
	//	case IBTrader::ITimeType::M1:
	//		return 5000;
	//		break;
	//	case IBTrader::ITimeType::M5:
	//		return 5000;
	//		break;
	//	case IBTrader::ITimeType::M15:
	//		return 5000;
	//		break;
	//	case IBTrader::ITimeType::M30:
	//		return 5000;
	//		break;
	//	case IBTrader::ITimeType::H1:
	//		return 5000;
	//		break;
	//	case IBTrader::ITimeType::D1:
	//		return 5000;
	//		break;
	//	default:
	//		break;
	//	}
	//}
	//return 1000;
}

SecurityType CTickUpdator::GetSecurityType(const std::string& codeId)
{
	if (codeId == "NQ" || codeId == "ES" || codeId == "MBT" || codeId == "ETHUSDRR")
	{
		return SecurityType::FUT;
	}
	else
	{
		return SecurityType::STK;
	}
}


