#pragma once

#include "DbTable_TickHis.h"
#include <QStruc.h>
// 数据库测试用例

void show_tick(const ITick& tick)
{
	printf("%s, time = %d, %d(%d) -> %d(%d) \n",
		tick.codeId.c_str(), int(tick.time), int(tick.bidAsks[0].bid), int(tick.bidAsks[0].bidVol),
		int(tick.bidAsks[0].ask), int(tick.bidAsks[0].askVol));

}
void show_ticks(const ITicks& ticks)
{
	for (ITicks::const_iterator pos = ticks.begin(); pos != ticks.end(); ++pos)
	{
		show_tick(*pos);
	}
}

IBidAsk init_bidask(const std::string& codeId, int data)
{
	IBidAsk bid_ask;
	bid_ask.bid = data;
	bid_ask.bidVol = data * 10;

	bid_ask.ask = data + 1;
	bid_ask.askVol = (data + 1) * 100;

	printf("%s bidask:  %.2f(%d) -> %.2f(%d) \n", codeId.c_str(), bid_ask.bid, bid_ask.bidVol, bid_ask.ask, bid_ask.askVol);
	return bid_ask;
}
ITicks init_ticks(const std::string& codeId, int beginTime, int spec, int count)
{
	ITicks back;
	int currentTime = beginTime;
	for (int i = 0; i < count; ++i)
	{
		ITick tick;
		tick.codeId = codeId;
		tick.time = currentTime;
		tick.bidAsks.push_back(init_bidask(codeId, int(tick.time)));
		back.push_back(tick);

		currentTime += spec;
	}
	return back;
}

void init_data(DbTable_TickHisPtr pTable)
{
	// 删除所有旧数据
	pTable->RemoveAll();

	// 开始添加新数据
	ITicks ticks = init_ticks("baba", 0, 2, 5);
	pTable->AddSome(ticks);

	ticks.clear();
	ticks = init_ticks("tencent", 20, 3, 5);
	pTable->AddSome(ticks);

	printf("init data over ----------------- \n");
}

void show_data_all(DbTable_TickHisPtr pTable, const std::string& codeId)
{
	IQuery query;
	query.byReqType = 0;
	query.dwSubscribeNum = 100000;

	ITicks ticks;
	pTable->GetTicks(codeId, query, ticks);
	printf("-- show all %s -- \n", codeId.c_str());
	show_ticks(ticks);
	printf("\n");
}
void show_data_one(DbTable_TickHisPtr pTable, const std::string& codeId, int timePos_exist, int timePos_noexist)
{
	ITick tick;
	bool success = pTable->GetOne(codeId, timePos_exist, tick);
	if (success)
	{
		printf("success, ind %s time = %d \n", codeId.c_str(), timePos_exist);
		ITicks ticks;
		ticks.push_back(tick);
		show_ticks(ticks);
	}
	else
	{
		printf("fail!!!, can not find %s time = %d \n", codeId.c_str(), timePos_exist);
	}

	success = pTable->GetOne(codeId, timePos_noexist, tick);
	if (success)
	{
		printf("fail!!!, ind %s time = %d \n", codeId.c_str(), timePos_noexist);
		ITicks ticks;
		ticks.push_back(tick);
		show_ticks(ticks);
	}
	else
	{
		printf("success, can not find %s time = %d \n", codeId.c_str(), timePos_noexist);
	}

}

void show_data_range(DbTable_TickHisPtr pTable, const std::string& codeId, int beginPos, int endPos)
{
	IQuery query;
	query.byReqType = 4;
	query.timePair.beginPos = beginPos;
	query.timePair.endPos = endPos;

	ITicks ticks;
	pTable->GetTicks(codeId, query, ticks);
	printf("-- show show_data_range -- \n");
	show_ticks(ticks);
	printf("\n\n\n");
}

void show_data_backward(DbTable_TickHisPtr pTable, const std::string& codeId, int pos, int count)
{
	IQuery query;
	query.byReqType = 2;
	query.tTime = pos;
	query.dwSubscribeNum = count;

	ITicks ticks;
	pTable->GetTicks(codeId, query, ticks);
	printf("-- show show_data_backward -- \n");
	show_ticks(ticks);
	printf("\n\n\n");

}
void show_data_forward(DbTable_TickHisPtr pTable, const std::string& codeId, int pos, int count)
{
	IQuery query;
	query.byReqType = 3;
	query.tTime = pos;

	ITicks ticks;
	pTable->GetTicks(codeId, query, ticks);
	printf("-- show show_data_forward -- \n");
	show_ticks(ticks);
	printf("\n\n\n");

}

void show_data_recent(DbTable_TickHisPtr pTable, const std::string& codeId, int count)
{
	IQuery query;
	query.byReqType = 0;
	query.dwSubscribeNum = count;

	ITicks ticks;
	pTable->GetTicks(codeId, query, ticks);
	printf("-- show show_data_recent -- \n");
	show_ticks(ticks);
	printf("\n\n\n");

}


