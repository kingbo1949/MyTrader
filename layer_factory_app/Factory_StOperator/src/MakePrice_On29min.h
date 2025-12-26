#pragma once
#include "MakePrice_Real.h"
class CMakePrice_On29min : public CMakePrice_Real
{
public:
	CMakePrice_On29min(const SubModuleParams& stParams);
	virtual ~CMakePrice_On29min() { ; };

protected:
	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override final;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override final;

	// 是否可以开仓 必须处于第29或59分钟才可以开仓
	bool					IsCanOpenTime();

	// 是否可以平仓 非止损平仓必须处于第31或01分钟才可以平仓
	bool					IsCanCoverTime();

	// 当前是否是二次开仓, 模拟使用1分钟线驱动，没有这个问题，实盘才有
	// 因为使用开盘价开仓，因此实盘中如果当前秒数太大则可判定是止损之后二次开仓
	bool					IsTwiceOpen();

	// 使用的用于得到高低价格的K线数目
	int						m_klinecount;

	HighAndLow				m_highAndLow;	// m_klinecount根BAR极值

	// 得到持仓价格，也是开仓价格
	double					GetPsPrice();

	// 是否需要止损
	bool					NeedStopCover(double& price);

	bool					IsRegularTradeTime();
};

