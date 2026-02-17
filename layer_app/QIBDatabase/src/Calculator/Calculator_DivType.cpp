#include "Calculator_DivType.h"
#include "../Factory.h"
#include <Global.h>
#include <Factory_Log.h>
CCalculator_DivType::CCalculator_DivType()
{
	const size_t longPeriod = 26;
	const size_t signalPeriod = 9;
	//const size_t circle = longPeriod + signalPeriod;
	m_circle = 26;
	m_uturnCircle = 11;

}
void CCalculator_DivType::Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines)
{
	IDivTypeValues ret(klines.size());
	for (int i = 0; i < klines.size(); ++i)
	{
		ret[i] = MakeOneInitValue();
		ret[i].time = klines[i].time;
	}

	for (auto i = 0; i < klines.size(); ++i)
	{
		if (i < m_circle)
		{
			ret[i].divType = IDivType::NORMAL;
			ret[i].isUTurn = false;
			continue;
		}

		size_t start = std::max(0, int(i - m_circle)); // 防止越界
		IKLines sub_klines(klines.begin() + start, klines.begin() + i);	// 选择[start, i)的BAR
		if (sub_klines.size() != m_circle)
		{
			printf("sub_klines.size() != m_circle \n");
			exit(-1);
		}
		ret[i].divType = ScanDivergenceType(codeId, timeType, sub_klines, klines[i]);

		// 选择[i-4, i]的BAR, 5根BAR
		sub_klines = std::vector<IKLine>(klines.begin() + (i - (m_uturnCircle - 1)), klines.begin() + i + 1);
		ret[i].isUTurn = IsUTurn(codeId, timeType, sub_klines, ret[i].divType);
	}
	UpdateValuesToDb(codeId, timeType, ret);


	//printf("divtypes begin: %s \n", CGlobal::GetTickTimeStr(ret[0].time).c_str());
	//printf("divtypes end: %s \n", CGlobal::GetTickTimeStr(ret.back().time).c_str());
	//printf("klines size = %d, divtypes size = %d \n", klines.size(), ret.size() );

	return;

}
void CCalculator_DivType::Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	time_t beginPos = benchmark_milliseconds();
	if (!Exist(codeId, timeType, newKline)) return;

	// 计算依赖于当前BAR和最近m_circle个MACD值
	IDivTypeValue ret = MakeOneInitValue();
	ret.time = newKline.time;

	// 取newKline之前的m_circle根BAR
	IKLines klines;
	IQuery query;
	query.byReqType = 2;
	query.tTime = newKline.time - 10;  // 不包括newKline
	query.dwSubscribeNum = m_circle;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timeType, query, klines);
	if (klines.size() != m_circle)
	{
		// 可能是第一次计算，或者数据不完整
		//printf("%s %s DivType::Update klines.size() != m_circle, newKline.time = %s  \n",
		//	codeId.c_str(),
		//	Trans_Str(timeType).c_str(),
		//	CGlobal::GetTickTimeStr(newKline.time).c_str()
		//	);
		//exit(-1);
		ret.divType = IDivType::NORMAL;
		ret.isUTurn = false;
		UpdateToDb(codeId, timeType, ret);
		return;

	}
	else
	{
		ret.divType = ScanDivergenceType(codeId, timeType, klines, newKline);
	}
	

	// 得到最后的m_uturnCircle根BAR
	IKLines klinesForUTurn = std::vector<IKLine>(klines.end() - (m_uturnCircle - 1), klines.end());
	klinesForUTurn.push_back(newKline);

	ret.isUTurn = IsUTurn(codeId, timeType, klinesForUTurn, ret.divType);

	UpdateToDb(codeId, timeType, ret);

	//time_t endPos = benchmark_milliseconds();
	//std::string str = fmt::format("{} {} update divtype used: {}", codeId.c_str(), Trans_Str(timeType).c_str(), int(endPos - beginPos));
	//Log_AsyncPrintDailyFile("updateindex", str, 1, false);

	return;

}
void CCalculator_DivType::UpdateValuesToDb(const std::string& codeId, ITimeType timeType, const IDivTypeValues& values)
{
	MakeAndGet_Env()->GetDB_DivType()->AddSome(codeId, timeType, values);

}

void CCalculator_DivType::UpdateToDb(const std::string& codeId, ITimeType timeType, const IDivTypeValue& value)
{
	MakeAndGet_Env()->GetDB_DivType()->AddOne(codeId, timeType, value);
	return;

}
CCalculator_DivType::HighAndLow CCalculator_DivType::MakeHighAndLow(const IKLines& klines, bool useOpenAndClose)
{
	HighAndLow highAndLow;
	for (const auto& kline : klines)
	{

		double high = 0;
		double low = 0;

		if (useOpenAndClose)
		{
			// 用开收盘代替BAR的高低价格，以去掉毛刺
			high = std::max(kline.open, kline.close);
			low = std::min(kline.open, kline.close);

		}
		else
		{
			high = kline.high;
			low = kline.low;

		}

		if (high > highAndLow.high)
		{
			highAndLow.highPos = kline.time;
			highAndLow.highPosStr = CGlobal::GetTickTimeStr(kline.time);
			highAndLow.high = high;
		}
		if (low < highAndLow.low)
		{
			highAndLow.lowPos = kline.time;
			highAndLow.lowPosStr = CGlobal::GetTickTimeStr(kline.time);
			highAndLow.low = low;
		}
	}
	return highAndLow;
}
IMacdValues CCalculator_DivType::GetMacdValues(const std::string& codeId, ITimeType timeType, const IKLines& klines)
{
	IMacdValues ret;
	IQuery query;
	query.byReqType = 4;
	query.timePair.beginPos = klines[0].time;
	query.timePair.endPos = klines.back().time + 1;
	MakeAndGet_Env()->GetDB_Macd()->GetValues(codeId, timeType, query, ret);
	return ret;
}

IMacdValue CCalculator_DivType::GetMacdValue(const std::string& codeId, ITimeType timeType, const IKLine& kline)
{
	IMacdValue ret;
	bool find = MakeAndGet_Env()->GetDB_Macd()->GetOne(codeId, timeType, kline.time, ret);
	if (!find)
	{
		printf("CCalculator_DivType_Real::GetMacdValue, not found \n");
		exit(-1);
	}
	return ret;
}
CCalculator_DivType::HighAndLow CCalculator_DivType::GetHighLow_MacdDif(const IMacdValues& values)
{
	HighAndLow ret;
	for (const auto& oneValue : values)
	{
		if (oneValue.dif > ret.high)
		{
			ret.high = oneValue.dif;
			ret.highPos = oneValue.time;
			ret.highPosStr = CGlobal::GetTickTimeStr(oneValue.time);
		}
		if (oneValue.dif < ret.low)
		{
			ret.low = oneValue.dif;
			ret.lowPos = oneValue.time;
			ret.lowPosStr = CGlobal::GetTickTimeStr(oneValue.time);
		}
	}

	return ret;
}
IDivTypeValue CCalculator_DivType::MakeOneInitValue()
{
	IDivTypeValue ret;
	ret.divType = IDivType::NORMAL;
	ret.isUTurn = false;
	return ret;
}

IDivType CCalculator_DivType::ScanDivergenceType(const std::string& codeId, ITimeType timeType, const IKLines& klines, const IKLine& kline)
{
	std::optional<bool> typeMain = CountDivType(codeId, timeType, klines, kline, MainOrSub::Main);
	std::optional<bool> typeSub = CountDivType(codeId, timeType, klines, kline, MainOrSub::Sub);

	if (typeMain.has_value())
	{
		if (*typeMain) return IDivType::TOP;
		return IDivType::BOTTOM;
	}
	if (typeSub.has_value())
	{
		if (*typeSub) return IDivType::TOPSUB;
		return IDivType::BOTTOMSUB;
	}

	return IDivType::NORMAL;
}

bool CCalculator_DivType::IsUTurn(const std::string& codeId, ITimeType timeType, const IKLines& klines, IDivType divType)
{
	if (divType == IDivType::NORMAL) return false;

	IMacdValues maceValues = GetMacdValues(codeId, timeType, klines);
	HighAndLow highAndLow = GetHighLow_MacdDif(maceValues);

	time_t extreamTime = klines[klines.size() - 2].time;	// 极值点的时间
	if (divType == IDivType::BOTTOM || divType == IDivType::BOTTOMSUB)
	{
		// 底背离
		if (highAndLow.lowPos == extreamTime)
		{
			return true;
		}
	}
	if (divType == IDivType::TOP || divType == IDivType::TOPSUB)
	{
		// 顶背离
		if (highAndLow.highPos == extreamTime)
		{
			return true;
		}
	}
	return false;
}

std::optional<bool> CCalculator_DivType::CountDivType(const std::string& codeId, ITimeType timeType, const IKLines& klines, const IKLine& kline, MainOrSub mainOrSub)
{
	HighAndLow highLow;
	if (mainOrSub == MainOrSub::Main)
	{
		highLow = MakeHighAndLow(klines, false);
	}
	else
	{
		highLow = MakeHighAndLow(klines, true);
	}

	if (highLow.high > kline.high && highLow.low < kline.low)
	{
		// 没有创新高或者新低
		return std::nullopt;
	}

	IMacdValues maceValues = GetMacdValues(codeId, timeType, klines);
	HighAndLow highAndLow = GetHighLow_MacdDif(maceValues);

	IMacdValue valueIndex = GetMacdValue(codeId, timeType, kline);

	if (highLow.high < kline.high)
	{
		// 价格创新高
		if (highAndLow.high > valueIndex.dif)
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
	if (highLow.low > kline.low)
	{
		// 价格创新低
		if (highAndLow.low < valueIndex.dif)
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
