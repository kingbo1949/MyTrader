#include "RecountKline.h"
#include <Factory_QDatabase.h>
#include <Factory_Log.h>
#include <Global.h>
void CRecountKline::Go()
{
	//QQuery query;
	//query.query_type = QQueryType::FROM_CURRENT;
	//query.query_number = 300;
	//IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine("NQ", Time_Type::M1, query);
	//printf("klines begin: %s \n", CGlobal::GetTickTimeStr(klines[0]->time).c_str() );
	//printf("klines end: %s \n", CGlobal::GetTickTimeStr(klines.back()->time).c_str());

	////query.query_type = QQueryType::BETWEEN_TIME;
	////query.time_pair.beginPos = klines[0]->time;
	////query.time_pair.endPos = klines.back()->time + 10;

	//IBDivTypePtrs divTypeValues = MakeAndGet_QDatabase()->GetDivTypes("NQ", Time_Type::M1, query);
	//printf("divtypes begin: %s \n", CGlobal::GetTickTimeStr(divTypeValues[0]->time).c_str());
	//printf("divtypes end: %s \n", CGlobal::GetTickTimeStr(divTypeValues.back()->time).c_str());


	RecountKLine("NQ");
	RecountKLine("ES");
	RecountKLine("TSLA");
	RecountKLine("NVDA");
	RecountKLine("AAPL");
	return;
}

void CRecountKline::RecountKLine(const CodeStr& codeId)
{
	//RecountKLine_Ma(codeId);
	//RecountKLine_Ema(codeId);
	//RecountKLine_Macd(codeId);
	RecountKLine_DivType(codeId);
}

void CRecountKline::RecountKLine_Ma(const CodeStr& codeId)
{
	Log_Print(LogLevel::Info, fmt::format("RecountKLine_Ma {} begin", codeId.c_str()));
	MakeAndGet_QDatabase()->RecountMa(codeId, Time_Type::S15);
	MakeAndGet_QDatabase()->RecountMa(codeId, Time_Type::M1);
	MakeAndGet_QDatabase()->RecountMa(codeId, Time_Type::M5);
	MakeAndGet_QDatabase()->RecountMa(codeId, Time_Type::M15);
	MakeAndGet_QDatabase()->RecountMa(codeId, Time_Type::M30);
	MakeAndGet_QDatabase()->RecountMa(codeId, Time_Type::H1);
	MakeAndGet_QDatabase()->RecountMa(codeId, Time_Type::D1);
	Log_Print(LogLevel::Info, fmt::format("RecountKLine_Ma {} over", codeId.c_str()));
	return;
}

void CRecountKline::RecountKLine_Ema(const CodeStr& codeId)
{
	Log_Print(LogLevel::Info, fmt::format("RecountKLine_Ema {} begin", codeId.c_str()));
	MakeAndGet_QDatabase()->RecountEma(codeId, Time_Type::S15);
	MakeAndGet_QDatabase()->RecountEma(codeId, Time_Type::M1);
	MakeAndGet_QDatabase()->RecountEma(codeId, Time_Type::M5);
	MakeAndGet_QDatabase()->RecountEma(codeId, Time_Type::M15);
	MakeAndGet_QDatabase()->RecountEma(codeId, Time_Type::M30);
	MakeAndGet_QDatabase()->RecountEma(codeId, Time_Type::H1);
	MakeAndGet_QDatabase()->RecountEma(codeId, Time_Type::D1);
	Log_Print(LogLevel::Info, fmt::format("RecountKLine_Ema {} over", codeId.c_str()));
	return;
}

void CRecountKline::RecountKLine_Macd(const CodeStr& codeId)
{
	Log_Print(LogLevel::Info, fmt::format("RecountKLine_Macd {} begin", codeId.c_str()));

	MakeAndGet_QDatabase()->RecountMacd(codeId, Time_Type::S15);
	MakeAndGet_QDatabase()->RecountMacd(codeId, Time_Type::M1);
	MakeAndGet_QDatabase()->RecountMacd(codeId, Time_Type::M5);
	MakeAndGet_QDatabase()->RecountMacd(codeId, Time_Type::M15);
	MakeAndGet_QDatabase()->RecountMacd(codeId, Time_Type::M30);
	MakeAndGet_QDatabase()->RecountMacd(codeId, Time_Type::H1);
	MakeAndGet_QDatabase()->RecountMacd(codeId, Time_Type::D1);
	Log_Print(LogLevel::Info, fmt::format("RecountKLine_Macd {} over", codeId.c_str()));
	return;
}

void CRecountKline::RecountKLine_DivType(const CodeStr& codeId)
{
	Log_Print(LogLevel::Info, fmt::format("RecountKLine_DivType {} begin", codeId.c_str()));
	MakeAndGet_QDatabase()->RecountDivType(codeId, Time_Type::S15);
	MakeAndGet_QDatabase()->RecountDivType(codeId, Time_Type::M1);
	MakeAndGet_QDatabase()->RecountDivType(codeId, Time_Type::M5);
	MakeAndGet_QDatabase()->RecountDivType(codeId, Time_Type::M15);
	MakeAndGet_QDatabase()->RecountDivType(codeId, Time_Type::M30);
	MakeAndGet_QDatabase()->RecountDivType(codeId, Time_Type::H1);
	MakeAndGet_QDatabase()->RecountDivType(codeId, Time_Type::D1);

	Log_Print(LogLevel::Info, fmt::format("RecountKLine_DivType {} over", codeId.c_str()));
	return;
}

