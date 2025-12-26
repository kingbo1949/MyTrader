#include "Calculator_Ma.h"
#include <numeric>
#include "Factory.h"
#include <Factory_Log.h>
void CCalculator_Ma::Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines)
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

void CCalculator_Ma::Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	time_t beginPos = benchmark_milliseconds();

	if (!Exist(codeId, timeType, newKline)) return;

	// 计算仅依赖于最近circle个BAR
	IAvgValue value;
	value.time = newKline.time;
	value.v5 = CountAverage(codeId, timeType, newKline, 5);
	value.v20 = CountAverage(codeId, timeType, newKline, 20);
	value.v60 = CountAverage(codeId, timeType, newKline, 60);
	value.v200 = CountAverage(codeId, timeType, newKline, 200);

	UpdateToDb(codeId, timeType, value);

	//time_t endPos = benchmark_milliseconds();
	//std::string str = fmt::format("{} {} update ma used: {}", codeId.c_str(), Trans_Str(timeType).c_str(), int(endPos - beginPos));
	//Log_AsyncPrintDailyFile("updateindex", str, 1, false);

	return;
}

void CCalculator_Ma::Initialize(const IKLines& klines, int circle, IAvgValues& values)
{
	auto sumClose = [](double acc, const IKLine& kline)
		{
			return acc + kline.close;
		};

	for (int i = 0; i < klines.size(); ++i)
	{
		// 得到指定需要填写位置的引用
		double& v = GetCircleValue(values[i], circle);
		if (i < circle - 1)
		{
			// 数据不足以计算均值
			v = std::numeric_limits<double>::quiet_NaN();
		}
		else
		{
			// 计算当前周期内的数据总和
			double sum = std::accumulate(klines.begin() + (i - circle + 1), klines.begin() + i + 1, 0.0, sumClose);
			// 计算平均值
			v = sum / circle;

		}
	}
}

double CCalculator_Ma::CountAverage(const std::string& codeId, ITimeType timeType, const IKLine& newKline, int circle)
{
	IQuery query;
	query.byReqType = 2;		// 从某时间往前取数据
	query.dwSubscribeNum = circle;
	query.tTime = newKline.time + 10;

	IKLines klines;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timeType, query, klines);
	if (klines.size() != circle)
	{
		if (timeType != ITimeType::D1)
		{
			printf("%s %s when update ma, klines do not have enough data, newKline.time = %s  \n",
				codeId.c_str(),
				Trans_Str(timeType).c_str(),
				CGlobal::GetTickTimeStr(newKline.time).c_str()
			);

		}
		return std::numeric_limits<double>::quiet_NaN();
	}
	auto sumClose = [](double acc, const IKLine& kline)
		{
			return acc + kline.close;
		};

	double sum = std::accumulate(klines.begin(), klines.begin() + circle, 0.0, sumClose);
	return sum / circle;

}


void CCalculator_Ma::UpdateToDb(const std::string& codeId, ITimeType timeType, const IAvgValue& value)
{
	MakeAndGet_Env()->GetDB_Ma()->AddOne(codeId, timeType, value);
	return;
}
