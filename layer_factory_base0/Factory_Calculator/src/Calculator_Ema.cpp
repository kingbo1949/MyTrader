#include "pch.h"
#include "Calculator_Ema.h"
#include <numeric>
CCalculator_Ema::CCalculator_Ema()
{
	m_circle_5 = 5;
	m_circle_20 = 20;
	m_circle_60 = 60;
	m_circle_200 = 200;

}

void CCalculator_Ema::Initialize(const IBKLinePtrs& klines)
{
	m_klines.clear();
	m_values.clear();
	m_valueMap.clear();


	m_klines = klines;
	m_values.resize(klines.size());
	for (auto i = 0; i < klines.size(); ++i)
	{
		Tick_T time = m_klines[i]->time;
		m_valueMap[time] = &m_values[i];
	}

	Initialize(m_klines, m_circle_5);
	Initialize(m_klines, m_circle_20);
	Initialize(m_klines, m_circle_60);
	Initialize(m_klines, m_circle_200);

	return;

}

void CCalculator_Ema::Update(IBKLinePtr newKline, bool isNewData)
{
	if (isNewData)
	{
		// 追加新数据
		m_klines.push_back(newKline);

		AverageValue value;
		value.v5 = CountLastEma(m_circle_5, isNewData);
		value.v20 = CountLastEma(m_circle_20, isNewData);
		value.v60 = CountLastEma(m_circle_60, isNewData);
		value.v200 = CountLastEma(m_circle_200, isNewData);

		m_values.push_back(value);
		m_valueMap[newKline->time] = &m_values.back();
	}
	else
	{
		if (m_values.empty()) return;

		m_klines.back() = newKline;

		m_values.back().v5 = CountLastEma(m_circle_5, isNewData);
		m_values.back().v20 = CountLastEma(m_circle_20, isNewData);
		m_values.back().v60 = CountLastEma(m_circle_60, isNewData);
		m_values.back().v200 = CountLastEma(m_circle_200, isNewData);
	}
	return;

}

AverageValue CCalculator_Ema::GetValueAtIndex(size_t index)
{
	if (index >= m_values.size())
	{
		return AverageValue(); // 数据不足或索引超出范围
	}
	return m_values[index];

}

AverageValue CCalculator_Ema::GetValueAtTime(Tick_T time)
{
	auto pos = m_valueMap.find(time);
	if (pos == m_valueMap.end())
	{
		return AverageValue();
	}

	return *(pos->second);
}

void CCalculator_Ema::Initialize(const IBKLinePtrs& klines, int circle)
{
	if (klines.size() < circle)
	{
		return; // 数据不足
	}

	double alpha = 2.0 / (circle + 1);
	double initialShortEMA = std::accumulate(klines.begin(), klines.begin() + circle, 0.0, g_SumClose) / circle;
	for (int i = circle - 1; i < klines.size(); ++i)
	{
		double& v = g_GetCircleValue(m_values[i], circle);
		if (i == circle - 1)
		{
			v = initialShortEMA;
		}
		else
		{
			double& lastV = g_GetCircleValue(m_values[i - 1], circle);
			v = alpha * klines[i]->close + (1 - alpha) * lastV;
		}
	}
}

double CCalculator_Ema::CountLastEma(int circle, bool isNewData)
{
	double alpha = 2.0 / (circle + 1);
	if (isNewData)
	{
		double& lastEma = g_GetCircleValue(m_values.back(), circle);
		double ema = alpha * m_klines.back()->close + (1 - alpha) * lastEma;
		return ema;
	}
	else
	{
		size_t indexLast2 = m_values.size() - 2;
		double& lastEma = g_GetCircleValue(m_values[indexLast2], circle);
		double ema = alpha * m_klines.back()->close + (1 - alpha) * lastEma;
		return ema;

	}
}
