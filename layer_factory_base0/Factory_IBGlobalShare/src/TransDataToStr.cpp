#include "pch.h"
#include "TransDataToStr.h"
#include <Factory_Log.h>
#include "Factory_IBGlobalShare.h"
#include <Global.h>
std::string CTransDataToStr::GetStr_Print(IBTickPtr thisTick, double minimove)
{
	std::string temstr = fmt::format(
		"{} ,"						// 时间
		"{:.2f} ,"					// 开盘
		"{:.2f} ,"					// 最新价
		"{} ,"						// 累计成交量
		"{:.2f} ,"					// 累计成交金额
		"{:.2f}({})->{:.2f}({}),"	// 盘口
		,
		CGlobal::GetTickTimeStr(thisTick->time).c_str(),
		thisTick->open * minimove,
		thisTick->last * minimove,
		thisTick->totalVol,
		thisTick->turnOver,
		thisTick->bidAsks[0].bid * minimove, thisTick->bidAsks[0].bidVol, thisTick->bidAsks[0].ask * minimove, thisTick->bidAsks[0].askVol
	);

	return temstr;


}

std::string CTransDataToStr::GetStr_Csv(IBTickPtr thisTick, double minimove)
{
	std::string temstr = fmt::format(
		"{} ,"						// 时间
		"{:.4f} ,"					// 开盘
		"{:.4f} ,"					// 最新价
		"{} ,"						// 累计成交量
		"{:.4f} ,"					// 累计成交金额
		"{:.2f} ,"					// bid
		"{} ,"						// bidvol
		"-> ,"
		"{:.2f} ,"					// ask
		"{} ,"						// askvol
		,
		CGlobal::GetTickTimeStr(thisTick->time).c_str(),
		thisTick->open * minimove,
		thisTick->last * minimove,
		thisTick->totalVol,
		thisTick->turnOver,
		thisTick->bidAsks[0].bid * minimove,
		thisTick->bidAsks[0].bidVol,
		thisTick->bidAsks[0].ask * minimove,
		thisTick->bidAsks[0].askVol
	);
	return temstr;


}

std::string CTransDataToStr::GetTickDepthStr_Csv(IBTickPtr thisTick, double minimove)
{
	IBTickPtr tick = thisTick->Clone();
	const int depthSize = 5;		// 深度行情的深度必须是5
	size_t needadd = depthSize - tick->bidAsks.size();
	for (auto i = 0; i < needadd; ++i)
	{
		IBBidAsk bidask;
		tick->bidAsks.push_back(bidask);
	}
	std::string temstr = fmt::format(
		"{} ,"						// 时间
		"{:.4f} ,"					// 开盘
		"{:.4f} ,"					// 最新价
		"{} ,"						// 累计成交量
		"{:.4f} ,"					// 累计成交金额
		"{:.2f} ,"					// bid5
		"{} ,"						// bid5vol
		"{:.2f} ,"					// bid4
		"{} ,"						// bid4vol
		"{:.2f} ,"					// bid3
		"{} ,"						// bid3vol
		"{:.2f} ,"					// bid2
		"{} ,"						// bid2vol
		"{:.2f} ,"					// bid1
		"{} ,"						// bid1vol
		"-> ,"
		"{:.2f} ,"					// ask1
		"{} ,"						// ask1vol
		"{:.2f} ,"					// ask2
		"{} ,"						// ask2vol
		"{:.2f} ,"					// ask3
		"{} ,"						// ask3vol
		"{:.2f} ,"					// ask4
		"{} ,"						// ask4vol
		"{:.2f} ,"					// ask5
		"{} ,"						// ask5vol
		,
		CGlobal::GetTickTimeStr(tick->time).c_str(),
		tick->open * minimove,
		tick->last * minimove,
		tick->totalVol,
		tick->turnOver,
		tick->bidAsks[4].bid * minimove,
		tick->bidAsks[4].bidVol,
		tick->bidAsks[3].bid * minimove,
		tick->bidAsks[3].bidVol,
		tick->bidAsks[2].bid * minimove,
		tick->bidAsks[2].bidVol,
		tick->bidAsks[1].bid * minimove,
		tick->bidAsks[1].bidVol,
		tick->bidAsks[0].bid * minimove,
		tick->bidAsks[0].bidVol,
		tick->bidAsks[0].ask * minimove,
		tick->bidAsks[0].askVol,
		tick->bidAsks[1].ask * minimove,
		tick->bidAsks[1].askVol,
		tick->bidAsks[2].ask * minimove,
		tick->bidAsks[2].askVol,
		tick->bidAsks[3].ask * minimove,
		tick->bidAsks[3].askVol,
		tick->bidAsks[4].ask * minimove,
		tick->bidAsks[4].askVol
	);
	return temstr;
}


std::string CTransDataToStr::GetKlineStr_Csv(IBKLinePtr kline)
{
	std::string temstr = fmt::format(
		"{} ,"						// 时间
		"{:.2f} ,"					// 开盘
		"{:.2f} ,"					// 最高
		"{:.2f} ,"					// 最低
		"{:.2f} ,"					// close
		"{}"						// vol
		,
		CGlobal::GetTickTimeStr(kline->time).c_str(),
		kline->open,
		kline->high,
		kline->low,
		kline->close,
		kline->vol
	);

	return temstr;

}
IBKLinePtr CTransDataToStr::GetKlineStr_Csv(const std::string& str)
{
	if (str.find("time") != std::string::npos)
	{
		// 这是标题行
		return nullptr;
	}
	std::vector<std::string> strings = CGlobal::SplitStr(str, ",");
	if (strings.size() < 6) return nullptr;

	IBKLinePtr ret = std::make_shared<CIBKLine>();
	ret->time = CGlobal::GetTickTimeByStr(strings[0]);
	ret->open = std::stod(strings[1]);
	ret->high = std::stod(strings[2]);
	ret->low = std::stod(strings[3]);
	ret->close = std::stod(strings[4]);
	ret->vol = std::stoi(strings[5]);

	return ret;
}

std::string CTransDataToStr::GetMaStr_Csv(IBMaPtr maline)
{
	std::string temstr = fmt::format(
		"{} ,"						// 时间
		"{:.2f} ,"					// 5
		"{:.2f} ,"					// 10
		"{:.2f} ,"					// 20
		"{:.2f} ,"					// 30
		,
		CGlobal::GetTickTimeStr(maline->time).c_str(),
		maline->v5,
		maline->v20,
		maline->v60,
		maline->v200
	);
	return temstr;

}

std::string CTransDataToStr::GetKlineTitle_Csv()
{
	return "time,open,high,low,close,vol";
}

std::string CTransDataToStr::GetTickTitle_Csv()
{
	std::string ticktitle = "time, open, close, totalvol, turnover, bid, bidvol, ->, ask, askvol,";
	return ticktitle;
}

std::string CTransDataToStr::GetDeepTickTitle_Csv()
{
	std::string ticktitle = "time, open, close, totalvol, turnover, bid5, bidvol5, bid4, bidvol4, bid3, bidvol3, bid2, bidvol2, bid1, bidvol1, ->, ask1, askvol1, ask2, askvol2, ask3, askvol3, ask4, askvol4, ask5, askvol5,";
	return ticktitle;
}


