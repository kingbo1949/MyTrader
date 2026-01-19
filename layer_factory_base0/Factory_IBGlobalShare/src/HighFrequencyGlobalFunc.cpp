#include "pch.h"
#include "HighFrequencyGlobalFunc.h"
#include "Factory_IBGlobalShare.h"
#include <Global.h>
#include <Factory_Log.h>
#include <iostream>
#include <fstream>


#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

time_t CHighFrequencyGlobalFunc::GetDayMillisec()
{
	time_t seconds = Get_CurrentTime()->GetCurrentTime_second();
	time_t back = CGlobal::QGetDayTime(seconds) * 1000;
	return back;

}

Tick_T CHighFrequencyGlobalFunc::GetDayMillisec(Tick_T tickTime)
{
	time_t seconds = tickTime / 1000;
	time_t back = CGlobal::QGetDayTime(seconds) * 1000;
	return back;

}

bool CHighFrequencyGlobalFunc::IsDaySession(Tick_T tickTime)
{
	Tick_T dayTime = GetDayMillisec(tickTime);
	Tick_T beginTime = dayTime + MakeMilliSecondPart("08:30:00", 0);
	Tick_T endTime = dayTime + MakeMilliSecondPart("16:00:00", 0);
	if (tickTime >= beginTime && tickTime < endTime) return true;

	return false;
}

bool CHighFrequencyGlobalFunc::IsAfter16(Tick_T tickTime)
{
	Tick_T dayTime = GetDayMillisec(tickTime);
	Tick_T nextTime = dayTime + 24 * 60 * 60 * 1000;

	Tick_T beginTime = dayTime + MakeMilliSecondPart("16:00:00", 0);
	if (tickTime >= beginTime && tickTime < nextTime) return true;
	return false;

}

int CHighFrequencyGlobalFunc::GetPriceStatus(IBTickPtr tick, BuyOrSell buyOrSell, int priceIn)
{
	int priceStep = priceIn;

	if (buyOrSell == BuyOrSell::Buy)
	{
		if (priceStep >= tick->bidAsks[0].ask)
		{
			// 对价吃成功
			int back = priceStep - tick->bidAsks[0].ask + 1;
			return -1 * back;
		}

		if (priceStep <= tick->bidAsks[0].bid)
		{
			// 挂单成功
			int back = tick->bidAsks[0].bid - priceStep + 1;
			return back;
		}

		// 挂单在队列最前面
		return 0;
	}
	else
	{
		if (priceStep <= tick->bidAsks[0].bid)
		{
			// 对价吃成功
			int back = tick->bidAsks[0].bid - priceStep + 1;
			return -1 * back;
		}

		if (priceStep >= tick->bidAsks[0].ask)
		{
			// 挂单成功
			int back = priceStep - tick->bidAsks[0].ask + 1;
			return back;
		}
		// 挂单在队列最前面
		return 0;

	}

}

int CHighFrequencyGlobalFunc::GetPriceByPriceStatus(IBTickPtr tick, BuyOrSell buyOrSell, int priceStatus)
{
	int back = 0;

	if (buyOrSell == BuyOrSell::Buy)
	{
		if (priceStatus > 0)
		{
			// 挂在委买
			back = tick->bidAsks[0].bid + 1 - priceStatus;
		}
		if (priceStatus == 0)
		{
			// 挂在最前
			back = tick->bidAsks[0].bid + 1;
			if (back == tick->bidAsks[0].ask)
			{
				back = tick->bidAsks[0].bid;	// 买卖盘太紧，0号位与1号位重合
			}

		}
		if (priceStatus < 0)
		{
			// 吃对价
			back = tick->bidAsks[0].ask - 1 - priceStatus;
		}

	}
	else
	{
		if (priceStatus > 0)
		{
			// 挂在委卖
			back = tick->bidAsks[0].ask - 1 + priceStatus;

		}
		if (priceStatus == 0)
		{
			// 挂在最前
			back = tick->bidAsks[0].ask - 1;
			if (back == tick->bidAsks[0].bid)
			{
				back = tick->bidAsks[0].ask;	// 买卖盘太紧，0号位与1号位重合
			}

		}
		if (priceStatus < 0)
		{
			// 吃对价
			back = tick->bidAsks[0].bid + 1 + priceStatus;

		}

	}

	return back;

}

int CHighFrequencyGlobalFunc::GetPriceByMidPoint(IBTickPtr tick, BuyOrSell buyOrSell)
{
	int back = 0;

	int diffstep = tick->bidAsks[0].ask - tick->bidAsks[0].bid;
	if (buyOrSell == BuyOrSell::Buy)
	{
		if (diffstep == 1)
		{
			// 盘口仅相差一个价位，返回0号位置
			return tick->bidAsks[0].bid;
		}
		return tick->bidAsks[0].bid + diffstep / 2;

	}
	else
	{
		if (diffstep == 1)
		{
			// 盘口仅相差一个价位，返回0号位置
			return tick->bidAsks[0].ask;
		}
		return tick->bidAsks[0].ask - diffstep / 2;
	}
	return tick->bidAsks[0].ask;
}


Tick_T CHighFrequencyGlobalFunc::MakeLastCloseTime()
{
	// 将下午4点计为每天最后收盘时间
	Tick_T dayToClose = 16 * 3600 * 1000;				// 从零点到收盘的毫秒
	Tick_T todayClose = GetDayMillisec() + dayToClose;

	Second_T time1 = Get_CurrentTime()->GetCurrentTime_second();
	tm tt;
	CGlobal::MyLocalTime(&time1, &tt);
	if (tt.tm_wday == 0)
	{
		// 周日
		return todayClose - 2 * 24 * 3600 * 1000;
	}
	if (tt.tm_wday == 6)
	{
		// 周六
		return todayClose - 1 * 24 * 3600 * 1000;
	}
	if (tt.tm_wday == 1)
	{
		// 周一
		if (time1 * 1000 >= todayClose)
		{
			return todayClose;
		}
		else
		{
			return todayClose - 3 * 24 * 3600 * 1000;
		}
	}
	// 其他时间
	if (time1 * 1000 >= todayClose)
	{
		return todayClose;
	}
	else
	{
		return todayClose - 24 * 3600 * 1000;
	}

}

HighAndLow CHighFrequencyGlobalFunc::MakeHighAndLow(const IBKLinePtrs& klines, bool useOpenAndClose)
{
	HighAndLow highAndLow;
	for (const auto& kline : klines)
	{

		double high = 0;
		double low = 0;

		if (useOpenAndClose)
		{
			// 用开收盘代替BAR的高低价格，以去掉毛刺
			high = std::max(kline->open, kline->close);
			low = std::min(kline->open, kline->close);

		}
		else
		{
			high = kline->high;
			low = kline->low;

		}

		if (high > highAndLow.high)
		{
			highAndLow.highPos = kline->time;
			highAndLow.highPosStr = CGlobal::GetTickTimeStr(kline->time);
			highAndLow.high = high;
		}
		if (low < highAndLow.low)
		{
			highAndLow.lowPos = kline->time;
			highAndLow.lowPosStr = CGlobal::GetTickTimeStr(kline->time);
			highAndLow.low = low;
		}
	}
	return highAndLow;
}

void CHighFrequencyGlobalFunc::PrintKLines(const IBKLinePtrs& klines)
{
	for (const auto& kline : klines)
	{
		std::string klinestr = CTransDataToStr::GetKlineStr_Csv(kline);
		Log_Print(LogLevel::Info, klinestr);
	}

}

IBKLinePtrs CHighFrequencyGlobalFunc::LoadKlinesFile(const CodeStr& codeId, Time_Type timeType)
{
	std::string filename = fmt::format("{}_kline_{}.csv",
		codeId.c_str(), CTransToStr::Get_TimeType(timeType));
	filename = "log/" + filename;

	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open the file " << filename << std::endl;
		exit(-1);
	}

	IBKLinePtrs klines;
	std::string line;

	// 读取每一行
	while (std::getline(file, line))
	{
		IBKLinePtr kline = CTransDataToStr::GetKlineStr_Csv(line);
		if (kline)
		{
			klines.push_back(kline);
		}

	}
	file.close();
	return klines;
}

