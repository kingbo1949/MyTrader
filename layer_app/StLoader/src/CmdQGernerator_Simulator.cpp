#include "CmdQGernerator_Simulator.h"
#include <Factory_QDatabase.h>
#include <Factory_IBJSon.h>
#include <Factory_Topics.h>
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_HashEnv.h>
#include <Global.h>
#include <Factory_AnalystDeal.h>
#include <Factory_Log.h>
#include <Log.h>
#include <Factory_StOperator.h>

#include <boost/filesystem.hpp>
using namespace boost::filesystem;

CCmdQGernerator_Simulator::CCmdQGernerator_Simulator()
{
	m_filepath = "simulator/";
	create_directory(m_filepath);
}
void CCmdQGernerator_Simulator::operator()()
{
	IBTickPtrs ticks;
	TimeZoneOfCodeIdPtr timeZone = MakeAndGet_JSonTimeZone(TimeZone_Type::For_Simulator)->Load_TimeZone();
	if (timeZone->lineType == LineType::UseKLine)
	{
		InitCalculators(*timeZone);
		ticks = GetTicksFromDb_Klines(*timeZone);
	}
	else
	{
		ticks = GetTicksFromDb_Ticks(*timeZone);
	}

	if (ticks.empty()) return;

	// 打印ticks描述
	PrintTicksDec(ticks);


	int count = 0;
	for (auto onetick : ticks)
	{
		count++;
		Get_CurrentTime()->SetCurrentTime(onetick->time * 1000);
		MakeAndGet_TradePointEnv()->CheckAndSetStatus();
		MakeAndGet_Topic_OnTick()->UpdateTick_OnTick(onetick);

		//if (count > 1) break;;
	}
	Log_Print(LogLevel::Info, fmt::format("thread {} is over, ticks size = {}", GetThreadName().c_str(), ticks.size()));


	// 查询并打印交易结果
	PrintAnalystResult();

}

std::string CCmdQGernerator_Simulator::GetThreadName()
{
	return "CmdQGernerator_Simulator";
}


IBTickPtrs CCmdQGernerator_Simulator::GetTicksFromDb_Ticks(const TimeZoneOfCodeId& timeZoneOfCodeId)
{
	;

	TimePair timePair;
	timePair.beginPos = timeZoneOfCodeId.beginPos;
	timePair.endPos = timeZoneOfCodeId.endPos;
	IBTickPtrs ticks = MakeAndGet_QDatabase()->GetTickHisByLoop(timeZoneOfCodeId.codeId, timePair);
	return ticks;

	// 按500毫秒4个价位来过滤tick
	//IBTickPtrs back = MakeAndGet_QDatabase()->FliteTicks(ticks, 500, 4);
	//return back;


}


IBTickPtrs CCmdQGernerator_Simulator::GetTicksFromDb_Klines(const TimeZoneOfCodeId& timeZoneOfCodeId)
{
	Log_Print(LogLevel::Info, fmt::format("will query klines, {} {} {}->{}",
		timeZoneOfCodeId.codeId.c_str(),
		CTransToStr::Get_TimeType(timeZoneOfCodeId.timeType).c_str(),
		CGlobal::GetTickTimeStr(timeZoneOfCodeId.beginPos).c_str(),
		CGlobal::GetTickTimeStr(timeZoneOfCodeId.endPos).c_str()

	));

	IBTickPtrs back;

	QQuery qQuery;
	qQuery.query_type = QQueryType::BETWEEN_TIME;
	qQuery.time_pair.beginPos = timeZoneOfCodeId.beginPos;
	qQuery.time_pair.endPos = timeZoneOfCodeId.endPos;

	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(timeZoneOfCodeId.codeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	double minimove = contract->minMove;
	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLineByLoop(timeZoneOfCodeId.codeId, timeZoneOfCodeId.timeType, qQuery.time_pair);
	for (auto onekline : klines)
	{
		IBTickPtr tick = std::make_shared<CIBTick>();
		tick->codeHash = codeHash;
		tick->time = onekline->time;

		IBBidAsk bidask;
		bidask.bid = CGlobal::DToI(onekline->close / minimove) - 10;
		bidask.bidVol = 1;
		bidask.ask = CGlobal::DToI(onekline->close / minimove) + 10;
		bidask.askVol = 1;


		tick->bidAsks.push_back(bidask);
		tick->timeStamp = onekline->time * 1000000;
		back.push_back(tick);
	}
	if (klines.empty())
	{
		Log_Print(LogLevel::Info, "query klines result, size = 0");
	}
	else
	{
		Log_Print(LogLevel::Info, fmt::format("query klines result, size = {}, {}->{}",
			klines.size(),
			CGlobal::GetTickTimeStr(klines[0]->time).c_str(),
			CGlobal::GetTickTimeStr(klines.back()->time).c_str()
		));

	}

	return back;

}

void CCmdQGernerator_Simulator::InitCalculators(TimeZoneOfCodeId timeZoneOfCodeId)
{
	// 把起始时间提前30天
	TimePair timePair;
	timePair.beginPos = timeZoneOfCodeId.beginPos - Tick_T(30) * 24 * 60 * 60 * 1000;
	timePair.endPos = timeZoneOfCodeId.endPos;

	InitCalculators(timeZoneOfCodeId.codeId, Time_Type::M1, timePair);
	InitCalculators(timeZoneOfCodeId.codeId, Time_Type::M5, timePair);
	InitCalculators(timeZoneOfCodeId.codeId, Time_Type::M15, timePair);
	InitCalculators(timeZoneOfCodeId.codeId, Time_Type::M30, timePair);
	InitCalculators(timeZoneOfCodeId.codeId, Time_Type::H1, timePair);
	InitCalculators(timeZoneOfCodeId.codeId, Time_Type::D1, timePair);


}

void CCmdQGernerator_Simulator::InitCalculators(const CodeStr& codeId, Time_Type timeType, TimePair time_pair)
{
	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLineByLoop(codeId, timeType, time_pair);
	MakeAndGet_Container_Ma()->Initialize(timeType, klines);
	MakeAndGet_Container_Macd()->Initialize(timeType, klines);
	MakeAndGet_Container_MacdDiv()->Initialize(timeType, klines);

	Log_Print(LogLevel::Info, fmt::format("InitCalculators klines, {} {} {}->{}, klines count = {}",
		codeId.c_str(),
		CTransToStr::Get_TimeType(timeType).c_str(),
		CGlobal::GetTickTimeStr(time_pair.beginPos).c_str(),
		CGlobal::GetTickTimeStr(time_pair.endPos).c_str(),
		klines.size()

	));


}

void CCmdQGernerator_Simulator::PrintTicksDec(const IBTickPtrs& ticks)
{
	if (ticks.empty())
	{
		Log_Print(LogLevel::Warn, "ticks size == 0");
		return;
	}
	IBTickPtr pfirst = ticks[0];
	IBTickPtr plast = ticks.back();
	Log_Print(LogLevel::Warn, fmt::format("{} -> {}, size = {}",
		CGlobal::GetTickTimeStr(pfirst->time).c_str(),
		CGlobal::GetTickTimeStr(plast->time).c_str(),
		ticks.size()
	));
	return;


}

void CCmdQGernerator_Simulator::PrintAnalystResult()
{
	double totalProfit = 0.0;
	double totalProfitRate = 0.0;
	size_t totalWinCount = 0;
	size_t totalLossCount = 0;
	size_t totalCount = 0;

	SimpleMatchPtrs allMatches;
	std::map<StrategyId, SimpleMatchPtrs> results = MakeAndGet_ScanDealsForSimpleMatchs()->Make_SimpleMatchs_FromJson();
	for (auto oneitem : results)
	{
		AnalystResultPtr analystResult = MakeAndGet_AnalystSimpleMatchs()->Go(oneitem.second);

		totalCount += analystResult->match_count;
		totalProfit += analystResult->profit;
		totalProfitRate += analystResult->profitRate;
		totalWinCount += analystResult->winCount;
		totalLossCount += analystResult->lossCount;

		analystResult->Print_Simple();

		// 写文件
		std::string fileName = oneitem.first + ".csv";
		fileName = m_filepath + fileName;
		analystResult->PrintToFile(fileName, oneitem.second, false);

		// 单策略按月打印结果
		PrintByMonth(oneitem.second);
		printf("============================================================================ \n");

		copy(oneitem.second.begin(), oneitem.second.end(), back_inserter(allMatches));

	}
	if (totalCount != 0)
	{
		Log_Print(LogLevel::Info, fmt::format("totalCount = {}, wincount = {}, losscount = {}, winRate = {:.2f}%",
			totalCount,
			totalWinCount,
			totalLossCount,
			(double)totalWinCount / totalCount * 100
		));
		Log_Print(LogLevel::Info, fmt::format("totalProfit = {}, profit/count = {}",
			totalProfit,
			totalProfit / totalCount
		));
		Log_Print(LogLevel::Info, fmt::format("totalProfitRate = {:.2f}%, profitRate/count = {:.2f}%",
			totalProfitRate * 100,
			totalProfitRate / totalCount * 100
		));
		// 所有策略按月打印结果
		PrintByMonth(allMatches);

	}
}




int CCmdQGernerator_Simulator::TransTimeTypeToPos(Time_Type timeType)
{
	int back = 0;
	switch (timeType)
	{
	case Time_Type::M1:
		back = 1 * 60 * 1000;
		break;
	case Time_Type::M5:
		back = 5 * 60 * 1000;
		break;
	case Time_Type::M15:
		back = 15 * 60 * 1000;
		break;
	case Time_Type::M30:
		back = 30 * 60 * 1000;
		break;
	case Time_Type::H1:
		back = 60 * 60 * 1000;
		break;
	case Time_Type::D1:
		back = 24 * 3600 * 1000;
		break;
	default:
		break;
	}
	return back;
}

void CCmdQGernerator_Simulator::PrintByMonth(const SimpleMatchPtrs& totalMatches)
{
	std::map<Tick_T, SimpleMatchPtrs> month_matches;
	SplitMatchsByMonth(totalMatches, month_matches);
	for (const auto& matchesMonth : month_matches)
	{
		AnalystResultMonthPtr pMonth = MakeAndGet_AnalystSimpleMatchsMonth(matchesMonth.second);
		pMonth->Go();
		std::string str = pMonth->PrintResult();
		Log_Print(LogLevel::Info, str);
	}

}
