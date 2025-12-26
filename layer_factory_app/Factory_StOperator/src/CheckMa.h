#pragma once

// 如果每一个tick都通过查询数据库去检查ma，过于耗时，因此通过这个类，每天只检查一次即可
#include <base_trade.h>
struct MaResult
{
	time_t							maDayTime = 0;		// 当日日期零点 maValue当中存放着不计算当日的前N日的MA值

	double							ma_d1_5 = 0.0;		// 5周期日线均值
	IBKLinePtrs						klines_d1;			// 用于计算MA的K线

	double							ma_h1_20;			// 20周期1小时均值
	double							ma_h1_60;			// 60周期1小时均值
};
typedef std::shared_ptr<MaResult> MaResultPtr;

class CCheckMa
{
public:
	CCheckMa(StrategyParamPtr pStrategyParam);
	virtual ~CCheckMa() { ; };

	// 检查是否可用，相当于一个初始化，每次都要先调用本函数，才能调用类的其他函数
	bool						Check(time_t ms);

	// 得到上日收盘价
	double						GetLastDayClose();

	// 得到上日最高价
	double						GetLastDayHigh();

	// 得到上日最低价
	double						GetLastDayLow();

	virtual TrendType			GetTrendType() = 0;

protected:
	StrategyParamPtr			m_pStrategyParam;
	MaResultPtr					m_pMaResult;

	bool						FillMaResult_DayPart(time_t ms);
	bool						FillMaResult_1HourPart(time_t ms);
};
typedef std::shared_ptr<CCheckMa> CheckMaPtr;

