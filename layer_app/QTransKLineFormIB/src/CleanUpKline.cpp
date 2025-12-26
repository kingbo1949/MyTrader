#include "CleanUpKline.h"
#include <Global.h>
#include <iostream>
#include <iomanip>
#include <sstream>
void CCleanUpKline::Go()
{
	ClenaUpKLine("NQ");
	ClenaUpKLine("ES");
	ClenaUpKLine("TSLA");
	ClenaUpKLine("NVDA");
	ClenaUpKLine("AAPL");
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
