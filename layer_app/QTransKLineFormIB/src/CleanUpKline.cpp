#include "CleanUpKline.h"
#include <Global.h>
#include <iostream>
#include <iomanip>
#include <sstream>
void CCleanUpKline::Go()
{
	Tick_T beginTime = 0;
	Tick_T endTime = LLONG_MAX;
	MakeAndGet_QDatabase()->DelCodeId("COST", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("ETHUSDRR", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("CRCL", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("HIMS", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("ALAB", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("KTOS", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("UPST", beginTime, endTime );


	// ClenaUpKLine("COST");
	// ClenaUpKLine("ETHUSDRR");
	// ClenaUpKLine("CRCL");
	// ClenaUpKLine("HIMS");
	// ClenaUpKLine("ALAB");
	// ClenaUpKLine("KTOS");
	// ClenaUpKLine("UPST");
}

void CCleanUpKline::ClenaUpKLine(const CodeStr& codeId)
{
	ClenaUpKLine(codeId, Time_Type::S15);
	ClenaUpKLine(codeId, Time_Type::M1);
	ClenaUpKLine(codeId, Time_Type::M5);
	ClenaUpKLine(codeId, Time_Type::M15);
	ClenaUpKLine(codeId, Time_Type::M30);
	ClenaUpKLine(codeId, Time_Type::H1);
	ClenaUpKLine(codeId, Time_Type::D1);
}

void CCleanUpKline::ClenaUpKLine(const CodeStr& codeId, Time_Type timetype)
{
	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetInvalidKLines(codeId, timetype);
	for (auto kline : klines)
	{
		MakeAndGet_QDatabase()->RemoveOneKLine(codeId, timetype, kline->time);
	}
}
