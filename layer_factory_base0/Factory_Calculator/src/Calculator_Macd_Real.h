#pragma once
#include "Calculator_Macd.h"
class CCalculator_Macd_Real : public CCalculator_Macd
{
public:
	CCalculator_Macd_Real();
	virtual ~CCalculator_Macd_Real() { ; };

	virtual void            Initialize(const IBKLinePtrs& klines) override final;

	virtual void			Update(IBKLinePtr newKline, bool isNewData) override final;

	virtual MacdValue		GetMacdAtIndex(size_t index)  override final;
	virtual MacdValue		GetMacdAtTime(Tick_T time)  override final;


	// [xBeginPos, xEndPos)内m_difValues的高低点
	virtual void			GetHighLow(int xBeginPos, int xEndPos, double& high, double& low)  override final;

private:
	IBKLinePtrs						m_klines;
	MacdValues						m_values;
	std::map<Tick_T, MacdValue*>	m_valueMap;
	int								m_shortPeriod;
	int								m_longPeriod;
	int								m_signalPeriod;
	double							m_alphaShort;
	double							m_alphaLong;
	double							m_alphaSignal;

};

