#pragma once


#include "Calculator_Average.h"
class CCalculator_Ema : public CCalculator_Average
{
public:
	CCalculator_Ema();
	virtual ~CCalculator_Ema() { ; };

	virtual void            Initialize(const IBKLinePtrs& klines) override final;

	virtual void			Update(IBKLinePtr newKline, bool isNewData) override final;

	virtual AverageValue	GetValueAtIndex(size_t index) override final;

	virtual AverageValue	GetValueAtTime(Tick_T time) override final;

protected:
	IBKLinePtrs						m_klines;
	AverageValues					m_values;
	std::map<Tick_T, AverageValue*>	m_valueMap;
	int								m_circle_5;
	int								m_circle_20;
	int								m_circle_60;
	int								m_circle_200;

	void					Initialize(const IBKLinePtrs& klines, int circle);

	// 计算末尾均值
	double					CountLastEma(int circle, bool isNewData);




};


