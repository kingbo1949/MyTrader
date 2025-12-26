#include "pch.h"
#include "Calculator_Ma.h"

#include <numeric>

CCalculator_Ma::CCalculator_Ma()
{
	m_circle_5 = 5;
	m_circle_20 = 20;
	m_circle_60 = 60;
	m_circle_200 = 200;
}

void CCalculator_Ma::Initialize(const IBKLinePtrs& klines)
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
void CCalculator_Ma::Update(IBKLinePtr newKline, bool isNewData)
{
	if (isNewData)
	{
		// 追加新数据
		m_klines.push_back(newKline);

		AverageValue value;
		value.v5 = CountAverage(m_circle_5);
		value.v20 = CountAverage(m_circle_20);
		value.v60 = CountAverage(m_circle_60);
		value.v200 = CountAverage(m_circle_200);

		m_values.push_back(value);
		m_valueMap[newKline->time] = &m_values.back();
	}
	else
	{
		if (m_values.empty()) return;

		m_klines.back() = newKline;

		m_values.back().v5 = CountAverage(m_circle_5);
		m_values.back().v20 = CountAverage(m_circle_20);
		m_values.back().v60 = CountAverage(m_circle_60);
		m_values.back().v200 = CountAverage(m_circle_200);
	}
	return;

}

AverageValue CCalculator_Ma::GetValueAtIndex(size_t index)
{
	AverageValue ret;
	if (index >= m_values.size())
	{
		return ret; // 数据不足或索引超出范围
	}
	return m_values[index];
}

AverageValue CCalculator_Ma::GetValueAtTime(Tick_T time)
{
	AverageValue ret;
	auto pos = m_valueMap.find(time);
	if (pos == m_valueMap.end())
	{
		return ret;
	}

	return *(pos->second);
}

void CCalculator_Ma::Initialize(const IBKLinePtrs& klines, int circle)
{
	if (klines.size() < circle)
	{
		return; // 数据不足
	}

	for (int i = circle - 1; i < klines.size(); ++i)
	{
		// 计算当前周期内的数据总和
		double sum = std::accumulate(klines.begin() + (i - circle + 1), klines.begin() + i + 1, 0.0, g_SumClose);
		// 计算平均值
		double& v = g_GetCircleValue(m_values[i], circle);
		v = sum / circle;
	}
}

double CCalculator_Ma::CountAverage(int circle)
{
	double sum = std::accumulate(m_klines.rbegin(), m_klines.rbegin() + circle, 0.0, g_SumClose);
	return sum / circle;
}

