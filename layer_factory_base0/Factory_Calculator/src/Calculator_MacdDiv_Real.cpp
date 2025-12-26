#include "pch.h"
#include "Calculator_MacdDiv_Real.h"

#include <algorithm>
#include <HighFrequencyGlobalFunc.h>
CCalculator_MacdDiv_Real::CCalculator_MacdDiv_Real(Calculator_MacdPtr macdCalculator)
	:m_macdCalculator(macdCalculator)
{
}

void CCalculator_MacdDiv_Real::Initialize(const IBKLinePtrs& klines)
{
	m_klines.clear();
	m_values.clear();
	m_valueMap.clear();

	m_klines = klines;
	m_values.resize(klines.size());
	std::fill(m_values.begin(), m_values.end(), DivergenceType::Normal);

	for (auto i = 0; i < m_klines.size(); ++i)
	{
		DivergenceType type = ScanDivergenceType(klines, i);
		m_values[i] = type;
		m_valueMap[m_klines[i]->time] = type;
	}

}

void CCalculator_MacdDiv_Real::Update(IBKLinePtr newKline, bool isNewData)
{
	if (isNewData)
	{
		// 追加新数据
		m_klines.push_back(newKline);

		DivergenceType type = ScanDivergenceType(m_klines, m_klines.size() - 1);
		m_values.push_back(type);
		m_valueMap[newKline->time] = type;
	}
	else
	{
		if (m_values.empty()) return;

		m_klines.back() = newKline;
		DivergenceType type = ScanDivergenceType(m_klines, m_klines.size() - 1);
		m_values.back() = type;
		m_valueMap[newKline->time] = type;
	}
	return;

}

DivergenceType CCalculator_MacdDiv_Real::GetMacdDivAtIndex(size_t index) const
{
	if (index >= m_values.size())
	{
		return DivergenceType::Normal; // 数据不足或索引超出范围
	}
	return m_values[index];

}

DivergenceType CCalculator_MacdDiv_Real::GetMacdDivAtTime(Tick_T time) const
{
	auto pos = m_valueMap.find(time);
	if (pos == m_valueMap.end())
	{
		return DivergenceType::Normal;
	}

	return pos->second;
}

DivergenceType CCalculator_MacdDiv_Real::ScanDivergenceType(const IBKLinePtrs& klines, size_t index)
{
	const size_t longPeriod = 26;
	const size_t signalPeriod = 9;
	//const size_t circle = longPeriod + signalPeriod;
	const size_t circle = 26;

	if (index < circle) return DivergenceType::Normal;

	size_t start = std::max(0, int(index - circle + 1)); // 防止越界
	IBKLinePtrs sub_klines(klines.begin() + start, klines.begin() + index); // 往前取circle - 1个数据，不包括index所在的元素
	HighAndLow highLowMain = CHighFrequencyGlobalFunc::MakeHighAndLow(sub_klines, false);
	HighAndLow highLowSub = CHighFrequencyGlobalFunc::MakeHighAndLow(sub_klines, true);

	std::optional<bool> typeMain = CountDivType(klines, start, index, highLowMain);
	std::optional<bool> typeSub = CountDivType(klines, start, index, highLowSub);

	if (typeMain.has_value())
	{
		if (*typeMain) return DivergenceType::Top;
		return DivergenceType::Bottom;
	}
	if (typeSub.has_value())
	{
		if (*typeSub) return DivergenceType::TopSub;
		return DivergenceType::BottomSub;
	}

	return DivergenceType::Normal;
}

std::optional<bool> CCalculator_MacdDiv_Real::CountDivType(const IBKLinePtrs& klines, size_t beginPos, size_t index, const HighAndLow& highLow)
{
	if (highLow.high > klines[index]->high && highLow.low < klines[index]->low)
	{
		// 没有创新高或者新低
		return std::nullopt;
	}

	MacdValues maceValues = GetMacdValues(beginPos, index); // index之前的macd数据
	HighLowPair highLowPair = GetHighLow_MacdDif(maceValues);

	MacdValue valueIndex = m_macdCalculator->GetMacdAtIndex(index);

	if (highLow.high < klines[index]->high)
	{
		// 价格创新高
		if (highLowPair.high > valueIndex.dif)
		{
			// 价格创新高dif却没有创新高 顶背离
			// 顶背离要求两线大于零
			return true;
			//if (valueIndex.dif > 0 && valueIndex.dea > 0)
			//{
			//	return true;
			//}

		}
	}
	if (highLow.low > klines[index]->low)
	{
		// 价格创新低
		if (highLowPair.low < valueIndex.dif)
		{
			// 价格创新高dif却没有创新低 底背离
			// 底背离要求两线小于零
			return false;
			//if (valueIndex.dif < 0 && valueIndex.dea < 0)
			//{
			//	return false;
			//}

		}
	}
	return std::nullopt;
}

MacdValues CCalculator_MacdDiv_Real::GetMacdValues(size_t beginPos, size_t endPos)
{
	MacdValues ret;
	for (size_t i = beginPos; i < endPos; ++i)
	{
		ret.push_back(m_macdCalculator->GetMacdAtIndex(i));
	}
	return ret;
}
HighLowPair CCalculator_MacdDiv_Real::GetHighLow_MacdDif(const MacdValues& values)
{
	HighLowPair ret;
	for (auto oneValue : values)
	{
		if (oneValue.dif > ret.high) ret.high = oneValue.dif;
		if (oneValue.dif < ret.low) ret.low = oneValue.dif;
	}

	return ret;
}

