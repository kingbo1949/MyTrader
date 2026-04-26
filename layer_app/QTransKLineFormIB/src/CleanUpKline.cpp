#include "CleanUpKline.h"
#include <Global.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <Factory_HashEnv.h>
void CCleanUpKline::Go()
{
	Tick_T beginTime = 0;
	Tick_T endTime = LLONG_MAX;
	MakeAndGet_QDatabase()->DelCodeId("C", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("MSTR", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("NFLX", beginTime, endTime );
	MakeAndGet_QDatabase()->DelCodeId("INTC", beginTime, endTime );


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
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	bool isIndex = (contract->securityType == SecurityType::INDEX);

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetInvalidKLines(codeId, isIndex, timetype);
	for (auto kline : klines)
	{
		MakeAndGet_QDatabase()->RemoveOneKLine(codeId, timetype, kline->time);
	}
}
