#include "Calculator_Ema.h"
#include <numeric>
#include "../Factory.h"
#include <Factory_Log.h>
#include <Global.h>
void CCalculator_Ema::Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines)
{
	IAvgValues ret(klines.size());
	for (auto i = 0; i < klines.size(); ++i)
	{
		ret[i].time = klines[i].time;
	}
	Initialize(klines, 5, ret);
	Initialize(klines, 20, ret);
	Initialize(klines, 60, ret);
	Initialize(klines, 200, ret);
	UpdateValuesToDb(codeId, timeType, ret);
	return;
}

void CCalculator_Ema::Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	time_t beginPos = benchmark_milliseconds();
	if (!Exist(codeId, timeType, newKline)) return;

	// 计算仅依赖于上一个BAR
	IAvgValue value;
	value.time = newKline.time;
	value.v5 = CountAverage(codeId, timeType, 5, newKline);
	value.v20 = CountAverage(codeId, timeType, 20, newKline);
	value.v60 = CountAverage(codeId, timeType, 60, newKline);
	value.v200 = CountAverage(codeId, timeType, 200, newKline);
	UpdateToDb(codeId, timeType, value);

	time_t endPos = benchmark_milliseconds();
	std::string str = fmt::format("{} {} update ema used: {}", codeId.c_str(), Trans_Str(timeType).c_str(), int(endPos - beginPos));
	Log_AsyncPrintDailyFile("updateindex", str, 1, false);

	return;
}

void CCalculator_Ema::Initialize(const IKLines& klines, int circle, IAvgValues& values)
{
	auto sumClose = [](double acc, const IKLine& kline)
		{
			return acc + kline.close;
		};
	double alpha = 2.0 / (circle + 1);
	for (int i = 0; i < klines.size(); ++i)
	{
		// 得到指定需要填写位置的引用
		double& v = GetCircleValue(values[i], circle);
		if (i < circle - 1)
		{
			// 数据不足以计算均值
			v = std::numeric_limits<double>::quiet_NaN();
			continue;
		}
		if (i == circle - 1)
		{
			double initialShortEMA = std::accumulate(klines.begin(), klines.begin() + circle, 0.0, sumClose) / circle;
			v = initialShortEMA;
			continue;
		}
		double& lastV = GetCircleValue(values[i - 1], circle);
		v = alpha * klines[i].close + (1 - alpha) * lastV;
	}
}

double CCalculator_Ema::CountAverage(const std::string& codeId, ITimeType timeType, int circle, const IKLine& newKline)
{
	double alpha = 2.0 / (circle + 1);

	IAvgValue emaValue = GetLastValue(codeId, timeType, newKline);
	double& lastEma = GetCircleValue(emaValue, circle);
	double ema = alpha * newKline.close + (1 - alpha) * lastEma;
	return ema;
}



IAvgValue CCalculator_Ema::GetLastValue(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	IQuery query;
	query.byReqType = 2;
	query.tTime = newKline.time - 1;
	query.dwSubscribeNum = 1;

	IEmaVaues values;
	MakeAndGet_Env()->GetDB_Ema()->GetValues(codeId, timeType, query, values);
	if (values.size() == 0)
	{
		printf("can not find ema GetLastValue \n");
		return IAvgValue();
	}
	return values[0];
}

void CCalculator_Ema::UpdateToDb(const std::string& codeId, ITimeType timeType, const IAvgValue& value)
{
	MakeAndGet_Env()->GetDB_Ema()->AddOne(codeId, timeType, value);
	return;

}
