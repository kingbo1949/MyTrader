#pragma once
// 指定TimePair之间的K线高低点形成一个通道
// 下穿通道下轨才可以开始做多，开仓基准线是下轨
// 上穿通道上轨才可以开始做空，开仓基准线是上轨
#include "BaseLine.h"
#include "CheckMa.h"
#include "HighLowPointOutside.h"
class CBaseLine_Channel : public CBaseLine
{
public:
	CBaseLine_Channel(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint);
	virtual ~CBaseLine_Channel() { ; };


protected:
	CheckMaPtr						m_pCheckMa;

	// 按动态行情重新刷新m_pricePair的值
	virtual bool					FreshPricePair(time_t now) override final;

	// m_pricePair已经存在，判断是否需要调整
	virtual bool					NeedFreshPricePair() override final;


	// 得到最近的盘外（盘后+盘前）K线
	IBKLinePtrs						GetKLines_TradeSide();

	// 得到最近的盘中K线
	IBKLinePtrs						GetKLines_TradeIn();

	// 取得盘前的高低价格
	HighAndLow						GetHighAndLow_TradeSide();

	time_t							GetLastDayTimeMs();

	CheckMaPtr						MakeAndGet_CheckMa();

	// 盘中行情必须穿越上下轨，激活之后，PricePair才可以使用，检查盘中行情是否可以激活PricePair
	bool							CanFreshPricePair(const HighAndLow& highAndLowTradeSide, const HighAndLow& highAndLowTradeIn);

	virtual double					TransParam(double param) override final;

	OneOrderPtr						GetDbLastOrder(StSubModule module);

	// 检查用于交易的高低点是否处于背离
	bool							IsMacdDiv(const HighAndLow& highAndLowTradeIn);

	// 得到极值点的背离状态
	DivergenceType					GetDivType(Time_Type timeType, const HighAndLow& highAndLowTradeIn);

	// 是否处于火车轨中
	bool							IsTrain(Time_Type timeType, const HighAndLow& highAndLowTradeIn);




};

