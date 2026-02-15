#include "Calculator_VwMa.h"
#include <numeric>
#include "../Factory.h"
#include <Factory_Log.h>
#include <Global.h>

void CCalculator_VwMa::Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines)
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

void CCalculator_VwMa::Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	time_t beginPos = benchmark_milliseconds();

	if (!Exist(codeId, timeType, newKline)) return;

	// ��������������circle��BAR
	IAvgValue value;
	value.time = newKline.time;
	value.v5 = CountAverage(codeId, timeType, newKline, 5);
	value.v20 = CountAverage(codeId, timeType, newKline, 20);
	value.v60 = CountAverage(codeId, timeType, newKline, 60);
	value.v200 = CountAverage(codeId, timeType, newKline, 200);

	UpdateToDb(codeId, timeType, value);

	time_t endPos = benchmark_milliseconds();
	std::string str = fmt::format("{} {} update ma used: {}", codeId.c_str(), Trans_Str(timeType).c_str(), int(endPos - beginPos));
	Log_AsyncPrintDailyFile("updateindex", str, 1, false);

	return;

}

void CCalculator_VwMa::Initialize(const IKLines& klines, int circle, IAvgValues& values)
{
	// �����ܳɽ���� = ���ͼ۸� * �ɽ���
	// ���ͼ۸� = (��߼� + ��ͼ� + ���̼�) / 3
	auto sumTurnOver = [](double acc, const IKLine& kline)
		{
			double typicalPrice = (kline.high + kline.low + kline.close) / 3.0;
			return acc + typicalPrice * kline.vol;
			
		};

	// ����ɽ���֮��
	auto sumVol = [](double acc, const IKLine& kline)
		{
			return acc + kline.vol;
		};


	for (int i = 0; i < klines.size(); ++i)
	{
		// �õ�ָ����Ҫ��дλ�õ�����
		double& v = GetCircleValue(values[i], circle);
		if (i < circle - 1)
		{
			// ���ݲ����Լ����ֵ
			v = std::numeric_limits<double>::quiet_NaN();
		}
		else
		{
			// ���㵱ǰ�����ڵ������ܺ�
			double sumT = std::accumulate(klines.begin() + (i - circle + 1), klines.begin() + i + 1, 0.0, sumTurnOver);
			double sumV = std::accumulate(klines.begin() + (i - circle + 1), klines.begin() + i + 1, 0.0, sumVol);
			// ����ƽ��ֵ
			v = sumT / sumV;

		}
	}

}

double CCalculator_VwMa::CountAverage(const std::string& codeId, ITimeType timeType, const IKLine& newKline, int circle)
{
	IQuery query;
	query.byReqType = 2;		// ��ĳʱ����ǰȡ����
	query.dwSubscribeNum = circle;
	query.tTime = newKline.time + 10;

	IKLines klines;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timeType, query, klines);
	if (klines.size() != circle)
	{
		printf("%s %s when update vwma, klines do not have enough data, newKline.time = %s  \n",
			codeId.c_str(),
			Trans_Str(timeType).c_str(),
			CGlobal::GetTickTimeStr(newKline.time).c_str()
		);
		return std::numeric_limits<double>::quiet_NaN();
	}

	auto sumTurnOver = [](double acc, const IKLine& kline)
		{
			double typicalPrice = (kline.high + kline.low + kline.close) / 3.0;
			return acc + typicalPrice * kline.vol;

		};

	// ����ɽ���֮��
	auto sumVol = [](double acc, const IKLine& kline)
		{
			return acc + kline.vol;
		};

	double sumT = std::accumulate(klines.begin(), klines.begin() + circle, 0.0, sumTurnOver);
	double sumV = std::accumulate(klines.begin(), klines.begin() + circle, 0.0, sumVol);
	return sumT / sumV;

}

void CCalculator_VwMa::UpdateToDb(const std::string& codeId, ITimeType timeType, const IAvgValue& value)
{
	MakeAndGet_Env()->GetDB_VwMa()->AddOne(codeId, timeType, value);
	return;

}
