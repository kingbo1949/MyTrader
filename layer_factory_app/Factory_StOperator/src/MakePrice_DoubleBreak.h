#pragma once
#include "MakePrice_Real.h"
class CMakePrice_DoubleBreak :  public CMakePrice_Real
{
public:
	CMakePrice_DoubleBreak(const SubModuleParams& stParams);
	virtual ~CMakePrice_DoubleBreak() { ; };

protected:
	// 使用的用于得到高低价格的K线数目
	int						m_klinecount_open;
	int						m_klinecount_cover;
	int						m_maxExternCount;			// 持仓最大延长次数
	int						m_externPsCount;			// 持仓被延长次数
	double					m_maxHighLowDiff;			// 开仓条件中，最近N根BAR最高最低差值



	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) override final;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) override final;

	// 反向突破平仓
	bool					GetCoverPriceByBreak(double& price, RealPriceType& priceType) ;

	// 按时间平仓
	bool					GetCoverPriceByTime(double& price, RealPriceType& priceType);

	// 是否可以平仓 非止损平仓必须处于开仓之后又过了count个周期
	bool					IsCanCoverTime(int count);

	// 自定义的交易时间段
	bool					IsRegularTradeTime();

	// 止损平仓
	bool					GetCoverPriceByStopLoss(double& price, RealPriceType& priceType);

	// 得到开仓价格
	bool					GetOpenPrice(double& price);

	// 检查持仓延长，如果被延长返回true
	bool					CheckPs();

	// 单纯依靠K线判断开仓形式，判断是否开仓，得到开仓价格
	// 开仓和是否需要持仓扩展，都需要用到这个函数
	bool					CheckOpenPrice(double& price, RealPriceType& priceType, double& highLowDiff);

	// 当前BAR持仓是否创新高或者新低
	bool					MakeGreatBreak();
};

