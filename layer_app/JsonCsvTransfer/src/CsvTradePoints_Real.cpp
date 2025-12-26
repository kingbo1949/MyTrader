#include "CsvTradePoints_Real.h"
#include <Log.h>
#include <Global.h>
#include <Factory_IBGlobalShare.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

void CCsvTradePoints_Real::LoadTradePointsFromCsv(const std::string& fileName, TradePointPtrs& tradePoints)
{
	printf("%s \n", fileName.c_str());

	MakeTitleIndex(fileName);


	std::string str = CLog::Instance()->ReadFileToString(fileName);
	std::vector<std::string> strs;
	split(strs, str, is_any_of("\n"), token_compress_off);
	for (const auto& oneLine : strs)
	{
		std::string lineStr = oneLine;
		trim(lineStr);
		if (lineStr == "") continue;
		if (IsTitleLine(lineStr, "primaryExchangePl")) continue;

		TradePointPtr tradePoint = MakeTradePoint(lineStr);
		if (!tradePoint) continue;

		tradePoints.push_back(tradePoint);
	}
	return;

}

TradePointPtr CCsvTradePoints_Real::MakeTradePoint(const std::string& csvStr)
{
	std::vector<std::string> strs;
	split(strs, csvStr, is_any_of(","), token_compress_off);
	strs = CGlobal::Trim(strs);

	if (strs.size() <= GetIndex("break_end"))
	{
		printf("cant split line:\n%s\n", csvStr.c_str());
		exit(-1);
	}
	if (strs[0] == "")
	{
		printf("drop line:[%s] \n", csvStr.c_str());
		return nullptr;
	}

	TradePointPtr back = std::make_shared<CTradePoint>();

	back->primaryExchangePl = CTransToStr::Get_ExchangePl(strs[GetIndex("primaryExchangePl")]);
	back->openMorning = strs[GetIndex("open_morning")];
	back->closeMorning = strs[GetIndex("close_morning")];

	back->openAfternoon = strs[GetIndex("open_afternoon")];
	back->closeAfternoon = strs[GetIndex("close_afternoon")];

	back->openEvening = strs[GetIndex("open_evening")];
	back->closeEvening = strs[GetIndex("close_evening")];

	back->breakBegin = strs[GetIndex("break_begin")];
	back->breakEnd = strs[GetIndex("break_end")];
	return back;

}
