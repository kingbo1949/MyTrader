#pragma once
#include "Factory_IBGlobalShare_Export.h"


#include <base_trade.h>


// 此类放在Factory_IBGlobalShare工程中，是因为他们很多函数需要使用CurrentTime类，因此不能放在baseLib工程中
class FACTORY_IBGLOBALSHARE_API CHighFrequencyGlobalFunc
{
public:
	CHighFrequencyGlobalFunc() { ; };
	virtual ~CHighFrequencyGlobalFunc() { ; };

	// 得到机器日的零点毫秒
	static Tick_T					GetDayMillisec();

	// 得到tickTime的零点毫秒
	static Tick_T					GetDayMillisec(Tick_T tickTime);

	// 判断是否是日盘
	static bool						IsDaySession(Tick_T tickTime);

	// hms的格式为HH:MM:SS, millis是毫秒零头
	inline static Tick_T			MakeMilliSecondPart(const char* hms, Tick_T millis)
	{
		std::string hhmmss(hms);
		if (hhmmss.length() == 7)
		{
			hhmmss = "0" + hhmmss;
		}

		if (hhmmss.length() != 8)
		{
			printf("%s !=8 \n", hhmmss.c_str());
			exit(-1);
		}

		// hms的格是为hh;mm:ss
		const int& hour = atoi(hhmmss.c_str());
		const int& minute = atoi(hhmmss.c_str() + 3);
		const int& second = atoi(hhmmss.c_str() + 6);
		return (hour * 3600 + minute * 60 + second) * 1000 + millis;
	}

	// 得到指定价格在盘口的位置（假设价格都是去买）
	// 返回-1表示吃对价委卖1，-2表示吃委卖2
	// 返回0，表示价格将排在最前面（比委买1要高，但是低于委卖1）
	// 返回1，表示价格讲挂在委买1，2表示挂在委买2
	static int 						GetPriceStatus(IBTickPtr tick, BuyOrSell buyOrSell, int priceIn);


	// 得到符合priceStatus的价格(0号位是指真正的最前面，不可以对价吃，如果买卖盘仅差一个价位，那么0号位将与1号位重合)
	static int 						GetPriceByPriceStatus(IBTickPtr tick, BuyOrSell buyOrSell, int priceStatus);

	// 得到盘口中间价
	static int 						GetPriceByMidPoint(IBTickPtr tick, BuyOrSell buyOrSell);


	// 按当前机器时间计算最近的上一个的收盘时间(上一个收盘时间不是上一日的收盘时间 毫秒)
	static Tick_T					MakeLastCloseTime();

	// 得到klines的高低价
	static HighAndLow				MakeHighAndLow(const IBKLinePtrs& klines, bool useOpenAndClose);

	// 打印K线
	static void						PrintKLines(const IBKLinePtrs& klines);

	// 从文件读取k线
	static IBKLinePtrs				LoadKlinesFile(const CodeStr& codeId, Time_Type timeType);


};

