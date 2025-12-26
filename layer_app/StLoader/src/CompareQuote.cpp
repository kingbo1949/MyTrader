#include "CompareQuote.h"
#include <GetRecordNo.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Factory_UnifyInterface.h>
#include <Global.h>
#include <Factory_Setup.h>
void CCompareQuote::Go()
{
	IbContractPtrs contracts = GetStrategyCodeId(CodeIdType::ForAll);

	size_t count = 5;
	for (auto contract : contracts)
	{
		CompareOneCode(contract->codeId, Time_Type::M1, count);
		CompareOneCode(contract->codeId, Time_Type::M30, count);
	}

}
void CCompareQuote::CompareOneCode(const std::string& codeId, Time_Type timeType, size_t count)
{
	IBKLinePtrs klines = GetKLine_FromIB_OneCode(codeId, timeType, count);
	for (const auto& kline : klines)
	{
		IBKLinePtr dbkline = MakeAndGet_QDatabase()->GetOneKLine(codeId, timeType, kline->time);
		if (!dbkline)
		{
			printf("dbkline is null \n");
			exit(-1);
		}
		std::string str = fmt::format("{} {} open:{:.2f}({:.2f}), high:{:.2f}({:.2f}), low:{:.2f}({:.2f}), close:{:.2f}({:.2f})",
			CTransToStr::Get_TimeType(timeType).c_str(),
			CGlobal::GetTimeStr(kline->time / 1000).c_str(),
			kline->open, dbkline->open,
			kline->high, dbkline->high,
			kline->low, dbkline->low,
			kline->close, dbkline->close
		);
		printf("%s \n", str.c_str() );
	}
	printf("\n");
}

IBKLinePtrs CCompareQuote::GetKLine_FromIB_OneCode(const std::string& codeId, Time_Type timeType, size_t count)
{
	bool onlyRegularTime = false;
	time_t now = CGetRecordNo::GetRecordNo(timeType, Get_CurrentTime()->GetCurrentTime_millisecond()) ;
	now = now / 1000;
	int days = 1;
	IBKLinePtrs klines = Get_IBApi()->QueryKLine(codeId, timeType, now, days, onlyRegularTime, m_useType);
	IBKLinePtrs back;
	reverse(klines.begin(), klines.end());

	time_t endPos = now * 1000 - 1;
	for (const auto& kline : klines)
	{
		if (kline->time > endPos) continue;
		back.push_back(kline);
		if (back.size() >= count) break;
	}
	reverse(back.begin(), back.end());
	return back;
}
