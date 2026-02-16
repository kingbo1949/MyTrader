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
	// UpdateOneCode("TSLA");
	UpdateOneCode("NQ");
	// UpdateOneCode("ES");
	// UpdateOneCode("NVDA");
	// UpdateOneCode("AAPL");
	
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
	UpdateFile(codeId, Time_Type::M1);
	UpdateFile(codeId, Time_Type::M5);
	UpdateFile(codeId, Time_Type::M15);
	UpdateFile(codeId, Time_Type::M30);
	UpdateFile(codeId, Time_Type::H1);
	UpdateFile(codeId, Time_Type::D1);

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
	printf("%s %s update over \n", codeId.c_str(), CTransToStr::Get_TimeType(timeType).c_str());


}
