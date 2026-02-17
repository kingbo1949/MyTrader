#include "UpdateKlineFromFile.h"
#include <Log.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Factory_Log.h>
using namespace boost::algorithm;

void CUpdateKlineFromFile::Go()
{
	UpdateOneCode("NQ");
	// UpdateOneCode("ES");
	// UpdateOneCode("GC");
	// UpdateOneCode("MBT");
	// UpdateOneCode("ETHUSDRR");

	// UpdateOneCode("NVDA");
	// UpdateOneCode("AMD");
	// UpdateOneCode("META");
	// UpdateOneCode("NFLX");
	// UpdateOneCode("MSFT");
	// UpdateOneCode("AMZN");
	// UpdateOneCode("COST");
	// UpdateOneCode("GOOG");
	// UpdateOneCode("TSM");
	// UpdateOneCode("INTC");
	// UpdateOneCode("HOOD");
	// UpdateOneCode("COIN");
	// UpdateOneCode("MSTR");
	// UpdateOneCode("CRCL");
	// UpdateOneCode("BMNR");
	// UpdateOneCode("PLTR");
	// UpdateOneCode("MU");
	// UpdateOneCode("WDC");
	// UpdateOneCode("STX");
	// UpdateOneCode("SOFI");
	// UpdateOneCode("VRT");
	// UpdateOneCode("GLXY");
	// UpdateOneCode("RKLB");
	// UpdateOneCode("ASTS");
	// UpdateOneCode("HIMS");
	// UpdateOneCode("ALAB");
	// UpdateOneCode("CLS");
	// UpdateOneCode("SNDK");
	// UpdateOneCode("KTOS");
	// UpdateOneCode("UPST");


}

std::string CUpdateKlineFromFile::GetFileName(const CodeStr& codeId, Time_Type timeType)
{
	std::string filename = fmt::format("./log/{}_kline_{}.csv",
		codeId.c_str(), CTransToStr::Get_TimeType(timeType)
	);
	return filename;
}

void CUpdateKlineFromFile::UpdateOneCode(const CodeStr& codeId)
{
	// UpdateFile(codeId, Time_Type::S15);

	// UpdateFile(codeId, Time_Type::M1);
	// UpdateFile(codeId, Time_Type::M5);
	// UpdateFile(codeId, Time_Type::M15);
	UpdateFile(codeId, Time_Type::M30);
	// UpdateFile(codeId, Time_Type::H1);
	// UpdateFile(codeId, Time_Type::D1);

}

void CUpdateKlineFromFile::UpdateFile(const CodeStr& codeId, Time_Type timeType)
{
	std::string filename = GetFileName(codeId, timeType);

	std::string str = CLog::Instance()->ReadFileToString(filename);
	std::vector<std::string> strs;
	split(strs, str, is_any_of("\n"), token_compress_off);
	IBKLinePtrs klines;
	for (const auto& oneLine : strs)
	{
		IBKLinePtr kline = CTransDataToStr::GetKlineStr_Csv(oneLine);
		if (!kline) continue;
		klines.push_back(kline);
	}
	// 更新到数据库
	for (auto kline : klines)
	{
		MakeAndGet_QDatabase()->UpdateKLine(codeId, timeType, kline);
	}
	MakeAndGet_QDatabase()->RecountAllIndex(codeId, timeType);


	printf("%s %s update over \n", codeId.c_str(), CTransToStr::Get_TimeType(timeType).c_str());

	CheckServerData(codeId, timeType);


}

void CUpdateKlineFromFile::CheckServerData(const CodeStr &codeId, Time_Type timeType)
{
	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 300;
	IBMaPtrs mas = MakeAndGet_QDatabase()->GetMas(codeId, timeType, query);

	return ;
}
