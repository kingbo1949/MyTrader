#include "PrintQuoteLastBar.h"
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_Log.h>
#include <Factory_Setup.h>
#include <Global.h>
#include <Factory_QShareEnv.h>
#include <Factory_HashEnv.h>
void CPrintQuoteLastBar::GoPrint()
{
	IbContractPtrs contracts = GetStrategyCodeId(CodeIdType::ForAll);

	for (auto contract : contracts)
	{
		PrintLastTick(contract);
		PrintLastBar(contract, Time_Type::M1);
		PrintLastBar(contract, Time_Type::M5);
		PrintLastBar(contract, Time_Type::M15);
		PrintLastBar(contract, Time_Type::M30);
		PrintLastBar(contract, Time_Type::H1);
		PrintLastBar(contract, Time_Type::D1);

		PrintRealTick(contract);
	}
	PrintLastTickToDb();
	
	return;
}

void CPrintQuoteLastBar::PrintLastTickToDb()
{
	int updateCount = 0;
	time_t recentUpdateSecnd = 0;
	IBTickPtr tick = MakeAndGet_QDatabase()->GetLastUpdateTick(updateCount, recentUpdateSecnd);
	if (!tick)
	{
		Log_Print(LogLevel::Warn, "no tick update to database");
		return;
	}
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(tick->codeHash);
	double minimove = contract->minMove;
	std::string tickstr = CTransDataToStr::GetStr_Print(tick, minimove);

	Log_Print(LogLevel::Info, fmt::format("\n\tlast tick update to db: {} count = {} \n\t {} {}",
		CGlobal::GetTimeStr(recentUpdateSecnd).c_str(),
		updateCount,
		contract->codeId.c_str(),
		tickstr.c_str()
		) );
}

void CPrintQuoteLastBar::PrintLastTick(IbContractPtr contract)
{
	IBTickPtr tick = MakeAndGet_QDatabase()->GetMarketQuoteDL(contract->codeId);
	if (!tick)
	{
		Log_Print(LogLevel::Warn, "tick is null");
		return;
	}
	if (tick->bidAsks.empty())
	{
		Log_Print(LogLevel::Warn, "tick bidasks is empty");
		return;
	}

	std::string tickstr = CTransDataToStr::GetStr_Print(tick, contract->minMove);
	Log_Print(LogLevel::Info, fmt::format("{} db {}", contract->codeId.c_str(), tickstr.c_str() ));
}

void CPrintQuoteLastBar::PrintLastBar(IbContractPtr contract, Time_Type timetype)
{
	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 1;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(contract->codeId, timetype, query);
	if (klines.empty()) return;

	std::string klinestr = CTransDataToStr::GetKlineStr_Csv(klines[0]);

	Log_Print(LogLevel::Info, fmt::format("{} {} {}",
		contract->codeId.c_str(), 
		CTransToStr::Get_TimeType(timetype).c_str(),
		klinestr.c_str()
	
	));


}

void CPrintQuoteLastBar::PrintRealTick(IbContractPtr contract)
{
	CodeHashId targetCode = Get_CodeIdEnv()->Get_CodeId_Hash(contract->codeId.c_str() );
	IBTickPtr ptick = MakeAndGet_QEnvManager()->GetEnvOneCode(targetCode)->GetTicEnv()->pTick;
	if (!ptick)
	{
		Log_Print(LogLevel::Warn, "real ptick is null");
		return;
	}

	std::string tickstr = CTransDataToStr::GetStr_Print(ptick, contract->minMove);
	Log_Print(LogLevel::Info, fmt::format("{} real {}", contract->codeId.c_str(), tickstr.c_str()));

}
