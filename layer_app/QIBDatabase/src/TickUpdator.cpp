#include "TickUpdator.h"
#include "Factory.h"
#include "GetRecordNoForDb.h"
#include "Calculator/Calculator.h"
#include <Factory_Log.h>
#include <Global.h>
#include "../cmd/Cmd_UpdateAllIndexFromTimePos.h"
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

	if (!exist)
	{
		// 新生成的K线 加入线程池立刻更新
		MakeAndGet_MyThreadPool()->commit(CCmd_UpdateAllIndexFromTimePos(tick.codeId, timeType, kline.time));
	}else
	{
		MakeAndGet_TimerTask_UpdateIndex()->AddNeedUpdate(tick.codeId, timeType);
	}


	return;
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


