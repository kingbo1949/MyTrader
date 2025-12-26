#include "pch.h"
#include "Calculator_Macd_Real.h"
#include <numeric>
CCalculator_Macd_Real::CCalculator_Macd_Real()
{
	m_shortPeriod = 12;
	m_longPeriod = 26;
	m_signalPeriod = 9;
	m_alphaShort = 2.0 / (m_shortPeriod + 1);
	m_alphaLong = 2.0 / (m_longPeriod + 1);
	m_alphaSignal = 2.0 / (m_signalPeriod + 1);

}

MacdValue CCalculator_Macd_Real::GetMacdAtIndex(size_t index)
{
	MacdValue result;
	if (index < m_longPeriod - 1 + m_signalPeriod - 1 || index >= m_values.size())
	{
		return result; // 数据不足或索引超出范围
	}
	result = m_values[index];
	return result;
}

MacdValue CCalculator_Macd_Real::GetMacdAtTime(Tick_T time)
{
	MacdValue ret;
	auto pos = m_valueMap.find(time);
	if (pos == m_valueMap.end()) return ret;
	if (!pos->second) return ret;

	ret = *(pos->second);
	return ret;
}


void CCalculator_Macd_Real::Initialize(const IBKLinePtrs& klines)
{
	if (klines.size() < m_longPeriod)
	{
		return; // 数据不足
	}
	m_klines.clear();
	m_values.clear();
	m_valueMap.clear();

	m_klines = klines;
	m_values.resize(klines.size());

	// 初始化EMA
	double initialShortEMA = std::accumulate(m_klines.begin(), m_klines.begin() + m_shortPeriod, 0.0, g_SumClose) / m_shortPeriod;
	double initialLongEMA = std::accumulate(m_klines.begin(), m_klines.begin() + m_longPeriod, 0.0, g_SumClose) / m_longPeriod;

	m_values[m_longPeriod - 1].emaShort = initialShortEMA;
	m_values[m_longPeriod - 1].emaLong = initialLongEMA;

	// 计算EMA
	for (int i = m_longPeriod; i < klines.size(); ++i)
	{
		m_values[i].emaShort = m_alphaShort * klines[i]->close + (1 - m_alphaShort) * m_values[i - 1].emaShort;
		m_values[i].emaLong = m_alphaLong * klines[i]->close + (1 - m_alphaLong) * m_values[i - 1].emaLong;
	}

	//计算DIF
	for (int i = m_longPeriod - 1; i < klines.size(); ++i)
	{
		m_values[i].dif = m_values[i].emaShort - m_values[i].emaLong;
	}
	//初始化DEA
	auto sumDif = [](double acc, const MacdValue& item)
		{
			return acc + item.dif;
		};
	double initialDEA = std::accumulate(m_values.begin() + m_longPeriod - 1, m_values.begin() + m_longPeriod - 1 + m_signalPeriod, 0.0, sumDif) / m_signalPeriod;
	m_values[m_longPeriod - 1 + m_signalPeriod - 1].dea = initialDEA;

	//计算DEA和MACD
	for (int i = m_longPeriod - 1 + m_signalPeriod; i < klines.size(); ++i)
	{
		m_values[i].dea = m_alphaSignal * m_values[i].dif + (1 - m_alphaSignal) * m_values[i - 1].dea;
		m_values[i].macd = m_values[i].dif - m_values[i].dea;
	}

	// 填充map
	for (int i = 0; i < klines.size(); ++i)
	{
		m_valueMap[klines[i]->time] = &(m_values[i]);
	}
	return;

}



void CCalculator_Macd_Real::Update(IBKLinePtr newKline, bool isNewData)
{
	auto sumDif = [](double acc, const MacdValue& item)
		{
			return acc + item.dif;
		};

	if (isNewData)
	{
		// 追加新数据
		m_klines.push_back(newKline);

		MacdValue value;
		value.emaShort = m_alphaShort * newKline->close + (1 - m_alphaShort) * m_values.back().emaShort;
		value.emaLong = m_alphaLong * newKline->close + (1 - m_alphaLong) * m_values.back().emaLong;
		value.dif = m_values.back().emaShort - m_values.back().emaLong;


		if (m_values.size() > m_longPeriod - 1 + m_signalPeriod - 1)
		{
			if (m_values.size() == m_longPeriod)
			{

				double initialDEA = std::accumulate(m_values.begin() + m_longPeriod - 1, m_values.begin() + m_longPeriod - 1 + m_signalPeriod, 0.0, sumDif) / m_signalPeriod;
				value.dea = initialDEA;
			}
			else
			{
				value.dea = m_alphaSignal * m_values.back().dif + (1 - m_alphaSignal) * m_values.back().dea;
			}
			value.macd = m_values.back().dif - m_values.back().dea;
			m_values.push_back(value);
		}
		m_valueMap[newKline->time] = &(m_values.back());

	}
	else
	{
		if (m_values.empty()) return;

		m_klines.back() = newKline;

		m_values.back().emaShort = m_alphaShort * newKline->close + (1 - m_alphaShort) * (m_values.size() > 1 ? m_values[m_values.size() - 2].emaShort : std::accumulate(m_klines.begin(), m_klines.begin() + m_shortPeriod, 0.0, g_SumClose) / m_shortPeriod);
		m_values.back().emaLong = m_alphaLong * newKline->close + (1 - m_alphaLong) * (m_values.size() > 1 ? m_values[m_values.size() - 2].emaLong : std::accumulate(m_klines.begin(), m_klines.begin() + m_longPeriod, 0.0, g_SumClose) / m_longPeriod);
		m_values.back().dif = m_values.back().emaShort - m_values.back().emaLong;

		if (m_values.size() > 0)
		{
			m_values.back().dea = m_alphaSignal * m_values.back().dif + (1 - m_alphaSignal) * (m_values.size() > 1 ? m_values[m_values.size() - 2].dea : std::accumulate(m_values.begin() + m_longPeriod - 1, m_values.begin() + m_longPeriod - 1 + m_signalPeriod, 0.0, sumDif) / m_signalPeriod);
			m_values.back().macd = m_values.back().dif - m_values.back().dea;
		}
	}
	return;
}


void CCalculator_Macd_Real::GetHighLow(int xBeginPos, int xEndPos, double& high, double& low)
{
	// xBeginPos和xEndPos是横轴所包含的时间窗口[xBeginPos, xEndPos)

	// 截取视窗内k线
	high = -99999999.9;
	low = 99999999.9;

	if (xBeginPos < 0)
	{
		xBeginPos = 0;
	}
	if (xEndPos >= int(m_values.size()))
	{
		xEndPos = int(m_values.size());
	}


	std::vector<MacdValue> values_in_window(m_values.begin() + xBeginPos, m_values.begin() + xEndPos);
	for (auto value : values_in_window)
	{
		if (value.dif > high) high = value.dif;
		if (value.dif < low) low = value.dif;
	}
	return;

}



