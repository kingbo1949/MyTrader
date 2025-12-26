#include "pch.h"
#include "AnalystResult_Real.h"
#include <Factory_Log.h>
#include <Global.h>
#include <Factory_HashEnv.h>
#include <Factory_QDatabase.h>
#include <Log.h>
void CAnalystResult_Real::Print_Simple()
{
	std::string str = fmt::format(
		"{} ,"						// 策略名称
		"wincount = {} ,"			// 盈利次数
		"losscount = {} ,"			// 亏损次数
		"match_count = {} ,"		// 总次数
		"winRate = {:.2f}% ,\n\t"	// 盈利比率
		"profit = {:.4f} ,"			// 总盈利
		"profitRate = {:.3f}% ,\n\t"// 总盈利率
		"profitPer = {:.4f} ,"		// 单比盈利
		"profitRatePer = {:.3f}% ,\n\t"// 单比盈利率
		"maxDropDown = {:.3f}% ,"	// 最大回撤
		"profitRiskRate = {:.3f} ,\n"// 报酬风险比
		"\t{} \n"					// 最大连续亏损次数
		"\t{} \n"					// 最大两个亏损
		"\t{} \n"					// 最大两个盈利
		,
		strategyId.c_str(),
		winCount,
		lossCount,
		match_count,
		winRate * 100,
		profit,
		profitRate * 100,
		profitPerMatch,
		profitRatePerMatch * 100,
		maxDropDown * 100,
		profitRiskRate,
		GetSeriesLoss_Simple().c_str(),
		GetFirstTwo(WinOrLoss::Loss, lossMatchs).c_str(),
		GetFirstTwo(WinOrLoss::Win, winMatchs).c_str()
	);


	Log_Print(LogLevel::Info, str);
}


void CAnalystResult_Real::PrintToFile(const std::string& fileName, const SimpleMatchPtrs& matchs, bool needDealTime)
{
	// 先删除文件
	CLog::Instance()->DelLogFile(fileName);
	std::string tital = "";
	if (needDealTime)
	{
		tital = "open, cover, openOrderTime, openDealTime, coverOrderTime, coverDealTime, profit, profitRate, sumprofitrate \n";
	}
	else
	{
		tital = "open, cover, openOrderTime, coverOrderTime, profit, profitRate, sumprofitrate \n";
	}

	
	CLog::Instance()->PrintStrToFile(fileName, tital);

	for (auto match : matchs)
	{
		std::string str = GetMatchStr_Csv(match, needDealTime) + "\n";
		CLog::Instance()->PrintStrToFile(fileName, str);
	}

}

std::string CAnalystResult_Real::GetSeriesLoss_Simple()
{
	if (seriesLoss.empty())
	{
		return "seriesLoss, count = 0";
	}

	std::string str = fmt::format("seriesLoss, count = {}, {}->{}",
		seriesLoss.size(),
		CGlobal::GetTickTimeStr(seriesLoss[0]->dealtime_open).c_str(),
		CGlobal::GetTickTimeStr(seriesLoss.back()->dealtime_cover).c_str()
	);
	return str;
}

std::string CAnalystResult_Real::GetFirstTwo(WinOrLoss winOrLoss, const SimpleMatchPtrs& matchs)
{
	std::string strProfit = "";
	size_t count = 2;
	for (size_t i = 0; i<matchs.size();++i)
	{
		if (i >= count)break;

		std::string matchstr = GetMatchStr(matchs[i]);
		strProfit += matchstr;
	}
	std::string dec = "";
	if (winOrLoss == WinOrLoss::Loss)
	{
		dec = "Top 2 Loss:";
	}
	else
	{
		dec = "Top 2 Win:";
	}

	std::string back = fmt::format("{}\n{}", dec.c_str(), strProfit.c_str());
	return back;
}

std::string CAnalystResult_Real::GetMatchStr(SimpleMatchPtr match)
{
	std::string str = fmt::format("\tprofitRate = {:.2f}%, profit = {}, {}->{}\n",
		match->profitRate * 100,
		match->profit,
		CGlobal::GetTickTimeStr(match->dealtime_open).c_str(),
		CGlobal::GetTickTimeStr(match->dealtime_cover).c_str()
		);
	return str;
}

std::string CAnalystResult_Real::GetMatchStr_Csv(SimpleMatchPtr match, bool needDealTime)
{
	std::string str = "";
	if (needDealTime)
	{
		// open, cover, openOrderTime, openDealTime, coverOrderTime, coverDealTime, profit, profitRate, sumprofitrate
		str = fmt::format("{:.2f}, {:.2f}, {}, {}, {}, {}, {:.3f}, {:.6f}, {:.6f}",
			match->price_open,
			match->price_cover,
			CGlobal::GetTickTimeStr(match->ordertime_open).c_str(),
			CGlobal::GetTickTimeStr(match->dealtime_open).c_str(),
			CGlobal::GetTickTimeStr(match->ordertime_cover).c_str(),
			CGlobal::GetTickTimeStr(match->dealtime_cover).c_str(),
			match->profit,
			match->profitRate,
			match->sumProfitRate
		);
	}
	else
	{
		// open, cover, openOrderTime, coverOrderTime, profit, profitRate, sumprofitrate
		str = fmt::format("{:.2f}, {:.2f}, {}, {}, {:.3f}, {:.6f}, {:.6f}",
			match->price_open,
			match->price_cover,
			CGlobal::GetTickTimeStr(match->ordertime_open).c_str(),
			CGlobal::GetTickTimeStr(match->ordertime_cover).c_str(),
			match->profit,
			match->profitRate,
			match->sumProfitRate
		);

	}

	return str;

}
