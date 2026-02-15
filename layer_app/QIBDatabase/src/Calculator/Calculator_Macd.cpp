#include "Calculator_Macd.h"
#include "../Factory.h"
#include <numeric>
#include <Factory_Log.h>
#include <Global.h>
CCalculator_Macd::CCalculator_Macd()
{
	m_shortPeriod = 12;
	m_longPeriod = 26;
	m_signalPeriod = 9;
	m_alphaShort = 2.0 / (m_shortPeriod + 1);
	m_alphaLong = 2.0 / (m_longPeriod + 1);
	m_alphaSignal = 2.0 / (m_signalPeriod + 1);

}
void CCalculator_Macd::Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines)
{
	if (klines.size() < m_longPeriod)
	{
		return;
	}

	IMacdValues ret(klines.size());
	for (int i = 0; i < klines.size(); ++i)
	{
		ret[i] = MakeOneInitValue();
		ret[i].time = klines[i].time;
	}
	auto sumClose = [](double acc, const IKLine& kline)
		{
			return acc + kline.close;
		};

	// 初始化EMA
	double initialShortEMA = std::accumulate(klines.begin(), klines.begin() + m_shortPeriod, 0.0, sumClose) / m_shortPeriod;
	double initialLongEMA = std::accumulate(klines.begin(), klines.begin() + m_longPeriod, 0.0, sumClose) / m_longPeriod;

	ret[m_longPeriod - 1].emaShort = initialShortEMA;
	ret[m_longPeriod - 1].emaLong = initialLongEMA;

	// 计算EMA
	for (int i = m_longPeriod; i < klines.size(); ++i)
	{
		ret[i].emaShort = m_alphaShort * klines[i].close + (1 - m_alphaShort) * ret[i - 1].emaShort;
		ret[i].emaLong = m_alphaLong * klines[i].close + (1 - m_alphaLong) * ret[i - 1].emaLong;
	}

	//计算DIF
	for (int i = m_longPeriod - 1; i < klines.size(); ++i)
	{
		ret[i].dif = ret[i].emaShort - ret[i].emaLong;
	}
	//初始化DEA
	auto sumDif = [](double acc, const IMacdValue& item)
		{
			return acc + item.dif;
		};
	double initialDEA = std::accumulate(ret.begin() + m_longPeriod - 1, ret.begin() + m_longPeriod - 1 + m_signalPeriod, 0.0, sumDif) / m_signalPeriod;
	ret[m_longPeriod - 1 + m_signalPeriod - 1].dea = initialDEA;

	//计算DEA和MACD
	for (int i = m_longPeriod - 1 + m_signalPeriod; i < klines.size(); ++i)
	{
		ret[i].dea = m_alphaSignal * ret[i].dif + (1 - m_alphaSignal) * ret[i - 1].dea;
		ret[i].macd = ret[i].dif - ret[i].dea;
	}

	// 更新到数据库
	UpdateValuesToDb(codeId, timeType, ret);
	return;

}
void CCalculator_Macd::Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	time_t beginPos = benchmark_milliseconds();
	if (!Exist(codeId, timeType, newKline)) return;

	// 计算仅依赖于当前BAR和上一个MACD值
	IMacdValue ret = MakeOneInitValue();
	ret.time = newKline.time;

	IMacdValue lastvalue = GetLastValue(codeId, timeType, newKline);
	ret.emaShort = m_alphaShort * newKline.close + (1 - m_alphaShort) * lastvalue.emaShort;
	ret.emaLong = m_alphaLong * newKline.close + (1 - m_alphaLong) * lastvalue.emaLong;
	ret.dif = ret.emaShort - ret.emaLong;
	ret.dea = m_alphaSignal * ret.dif + (1 - m_alphaSignal) * lastvalue.dea;
	ret.macd = ret.dif - ret.dea;

	UpdateToDb(codeId, timeType, ret);

	//time_t endPos = benchmark_milliseconds();
	//std::string str = fmt::format("{} {} update macd used: {}", codeId.c_str(), Trans_Str(timeType).c_str(), int(endPos - beginPos));
	//Log_AsyncPrintDailyFile("updateindex", str, 1, false);

	return;

}
IMacdValue CCalculator_Macd::MakeOneInitValue()
{
	IMacdValue value;
	value.emaShort = std::numeric_limits<double>::quiet_NaN();
	value.emaLong = std::numeric_limits<double>::quiet_NaN();
	value.dif = std::numeric_limits<double>::quiet_NaN();
	value.dea = std::numeric_limits<double>::quiet_NaN();
	value.macd = std::numeric_limits<double>::quiet_NaN();
	return value;
}


IMacdValue CCalculator_Macd::GetLastValue(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	IQuery query;
	query.byReqType = 2;
	query.tTime = newKline.time - 1;
	query.dwSubscribeNum = 1;

	IMacdValues values;
	MakeAndGet_Env()->GetDB_Macd()->GetValues(codeId, timeType, query, values);
	if (values.size() == 0)
	{
		printf("can not find macd GetLastValue \n");
		return IMacdValue();
	}
	return values[0];
}

void CCalculator_Macd::UpdateValuesToDb(const std::string& codeId, ITimeType timeType, const IMacdValues& values)
{
	for (const auto& value : values)
	{
		UpdateToDb(codeId, timeType, value);
	}

}

void CCalculator_Macd::UpdateToDb(const std::string& codeId, ITimeType timeType, const IMacdValue& value)
{
	MakeAndGet_Env()->GetDB_Macd()->AddOne(codeId, timeType, value);
	return;

}
