#pragma once
#include <base_struc.h>
#include <qcolor.h>
// 用不同的背景色标注盘后与夜盘时间
// 股票的盘后时间：16：00 - 20：00 夜盘时间：04：00 - 09：30
// 期指的盘后时间：16：00 - 17：00 夜盘上：18：00 - 04：00 夜盘下：04：00 - 09：30

enum class TradeSideType
{
	TradeIn,		// 盘中
	AfterTrade,		// 盘后交易
	NightStock,		// 股票夜盘
	NightFuture1,	// 期货夜盘第1段
	NightFuture2	// 期货夜盘第2段
};


// 一个区间[beginIndex, endIndex]
struct TSideRange
{
	int				beginIndex = -1;
	int				endIndex = -1;
	TradeSideType	tSideType = TradeSideType::TradeIn;
};
typedef std::vector<TSideRange> TSideRanges;

class CTSideTimeHandler
{
public:
	CTSideTimeHandler() { ; };
	virtual ~CTSideTimeHandler() { ; };

	// 扫描k线得到盘外区间
	TSideRanges				ScanKlines(const IBKLinePtrs& klines);

	// 得到指定区域的颜色
	virtual QColor			GetTSideColor(TradeSideType type) = 0;

protected:

	// 得到某一时点的区间
	virtual TradeSideType	GetTSideType(Tick_T time) = 0;
};
typedef std::shared_ptr<CTSideTimeHandler> TSideTimeHandlerPtr;

