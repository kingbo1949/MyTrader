#include "TickConverter.h"

#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Factory_QDatabase.h>
#include <Factory_Log.h>
#include <Global.h>
void CTickConverter::ConvertTicksFromIBToDb(const CodeStr& codeId, TimePair timePair)
{
	time_t beginDaySecond = CHighFrequencyGlobalFunc::GetDayMillisec(timePair.beginPos) / 1000;
	time_t endDaySecond = CHighFrequencyGlobalFunc::GetDayMillisec(timePair.endPos) / 1000;

	time_t secondInDay = 24 * 3600;
	for (time_t i = beginDaySecond; i <= endDaySecond; i += secondInDay)
	{
		ConvertTicksFromIBToDb(codeId, i);
	}
	return;
}

void CTickConverter::ConvertTicksFromIBToDb(const CodeStr& codeId, time_t daySecond)
{
	IBTickPtrs ticks_pre = Get_IBApi()->QueryTicks(codeId, daySecond, MarketType::PreMarket);
	IBTickPtrs ticks_post = Get_IBApi()->QueryTicks(codeId, daySecond, MarketType::AfterHour);

	if (ticks_pre.empty())
	{
		Log_Print(LogLevel::Info, fmt::format("will update tick to db, {} {} {} size = {} ",
			codeId.c_str(),
			CGlobal::GetTimeStr(daySecond).c_str(),
			CTransToStr::Get_MarketType(MarketType::PreMarket).c_str(),
			ticks_pre.size()
		));

	}
	else
	{
		Log_Print(LogLevel::Info, fmt::format("will update tick to db, {} {} {} size = {} {}->{}",
			codeId.c_str(),
			CGlobal::GetTimeStr(daySecond).c_str(),
			CTransToStr::Get_MarketType(MarketType::PreMarket).c_str(),
			ticks_pre.size(),
			CGlobal::GetTickTimeStr(ticks_pre[0]->time).c_str(),
			CGlobal::GetTickTimeStr(ticks_pre.back()->time).c_str()
		));

	}


	// 更新到数据库
	for (auto onetick : ticks_pre)
	{
		MakeAndGet_QDatabase()->UpdateTicks(onetick);
	}


	if (ticks_post.empty())
	{
		Log_Print(LogLevel::Info, fmt::format("will update tick to db, {} {} {} size = {}",
			codeId.c_str(),
			CGlobal::GetTimeStr(daySecond).c_str(),
			CTransToStr::Get_MarketType(MarketType::AfterHour).c_str(),
			ticks_post.size()
		));

	}
	else
	{
		Log_Print(LogLevel::Info, fmt::format("will update tick to db, {} {} {} size = {}, {}->{}",
			codeId.c_str(),
			CGlobal::GetTimeStr(daySecond).c_str(),
			CTransToStr::Get_MarketType(MarketType::AfterHour).c_str(),
			ticks_post.size(),
			CGlobal::GetTickTimeStr(ticks_post[0]->time).c_str(),
			CGlobal::GetTickTimeStr(ticks_post.back()->time).c_str()
		)); 

	}


	for (auto onetick : ticks_post)
	{
		MakeAndGet_QDatabase()->UpdateTicks(onetick);
	}

}

void CTickConverter::QueryTickInDb(const CodeStr& codeId, time_t daySecond, MarketType marketType)
{
	time_t beginSecond = 0;
	time_t endSecond = 0;
	if (marketType == MarketType::PreMarket)
	{
		// 04:00 - 09:30
		beginSecond = daySecond + 4 * 3600;
		endSecond = daySecond + 9 * 3600 + 30 * 60;
	}
	else
	{
		// 16:00 - 20:00
		beginSecond = daySecond + 16 * 3600;
		endSecond = daySecond + 20 * 3600;
	}

	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = beginSecond * 1000;
	query.time_pair.endPos = endSecond * 1000;

	IBTickPtrs ticksdb = MakeAndGet_QDatabase()->GetTickHis(codeId, query);
	if (ticksdb.empty())
	{
		Log_Print(LogLevel::Warn, fmt::format("{} {} {},ticks in db size = {}",
			codeId.c_str(),
			CGlobal::GetTimeStr(daySecond).c_str(),
			CTransToStr::Get_MarketType(marketType).c_str(),
			ticksdb.size()
		));

	}
	else
	{
		Log_Print(LogLevel::Info, fmt::format("{} {} {}, ticks in db size = {}, {} -- {}",
			codeId.c_str(),
			CGlobal::GetTimeStr(daySecond).c_str(),
			CTransToStr::Get_MarketType(marketType).c_str(),
			ticksdb.size(),
			CGlobal::GetTickTimeStr(ticksdb[0]->time).c_str(),
			CGlobal::GetTickTimeStr(ticksdb.back()->time).c_str()
		));

	}


}

void CTickConverter::QueryTickInDb(const CodeStr& codeId, TimePair timePair)
{
	time_t beginDaySecond = CHighFrequencyGlobalFunc::GetDayMillisec(timePair.beginPos) / 1000;
	time_t endDaySecond = CHighFrequencyGlobalFunc::GetDayMillisec(timePair.endPos) / 1000;

	time_t secondInDay = 24 * 3600;
	for (time_t i = beginDaySecond; i <= endDaySecond; i += secondInDay)
	{
		QueryTickInDb(codeId, i, MarketType::PreMarket);
		QueryTickInDb(codeId, i, MarketType::AfterHour);
	}
	return;

}

void CTickConverter::RequestKLineInDb(const CodeStr& codeId, time_t daySecond, MarketType marketType)
{
	time_t beginSecond = 0;
	time_t endSecond = 0;
	if (marketType == MarketType::PreMarket)
	{
		// 04:00 - 09:30
		beginSecond = daySecond + 4 * 3600;
		endSecond = daySecond + 9 * 3600 + 30 * 60;
	}
	else
	{
		// 16:00 - 20:00
		beginSecond = daySecond + 16 * 3600;
		endSecond = daySecond + 20 * 3600;
	}

	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = beginSecond * 1000;
	query.time_pair.endPos = endSecond * 1000;

	IBTickPtrs ticksdb = MakeAndGet_QDatabase()->GetTickHis(codeId, query);
	if (ticksdb.empty())
	{
		Log_Print(LogLevel::Warn, fmt::format("{} {} {}, ticks in db size = {}",
			codeId.c_str(),
			CGlobal::GetTimeStr(daySecond).c_str(),
			CTransToStr::Get_MarketType(marketType).c_str(),
			ticksdb.size()
		));

	}
	else
	{
		for (auto onetick : ticksdb)
		{
			Log_Print(LogLevel::Info, fmt::format("{} last = {}, lastsize = {}, {}({})->{}({])",
				CGlobal::GetTickTimeStr(onetick->time).c_str(),
				onetick->last,
				onetick->vol,
				onetick->bidAsks[0].bid,
				onetick->bidAsks[0].bidVol,
				onetick->bidAsks[0].ask,
				onetick->bidAsks[0].askVol
			));
		}
	}


}
