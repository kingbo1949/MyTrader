#include "Calculator.h"

#include "Calculator_Ma.h"
#include "Calculator_VwMa.h"
#include "Calculator_Ema.h"
#include "Calculator_Macd.h"
#include "Calculator_DivType.h"
#include "Calculator_Atr.h"
#include "../Factory.h"

CalculatorPtr g_calculator_ma = nullptr;
CalculatorPtr MakenAndGet_Calculator_Ma()
{
	if (!g_calculator_ma)
	{
		g_calculator_ma = std::make_shared<CCalculator_Ma>();
	}
	return g_calculator_ma;
}

CalculatorPtr g_calculator_vwma = nullptr;
CalculatorPtr MakenAndGet_Calculator_VwMa()
{
	if (!g_calculator_vwma)
	{
		g_calculator_vwma = std::make_shared<CCalculator_VwMa>();
	}
	return g_calculator_vwma;
}
CalculatorPtr g_calculator_ema = nullptr;
CalculatorPtr MakenAndGet_Calculator_Ema()
{
	if (!g_calculator_ema)
	{
		g_calculator_ema = std::make_shared<CCalculator_Ema>();
	}
	return g_calculator_ema;
}
CalculatorPtr g_calculator_macd = nullptr;
CalculatorPtr MakenAndGet_Calculator_Macd()
{
	if (!g_calculator_macd)
	{
		g_calculator_macd = std::make_shared<CCalculator_Macd>();
	}
	return g_calculator_macd;
}
CalculatorPtr g_calculator_divtype = nullptr;
CalculatorPtr MakenAndGet_Calculator_DivType()
{
	if (!g_calculator_divtype)
	{
		g_calculator_divtype = std::make_shared<CCalculator_DivType>();
	}
	return g_calculator_divtype;
}
CalculatorPtr g_calculator_atr = nullptr;
CalculatorPtr MakenAndGet_Calculator_Atr()
{
	if (!g_calculator_atr)
	{
		g_calculator_atr = std::make_shared<CCalculator_Atr>();
	}
	return g_calculator_atr;

}

bool CCalculator::Exist(const std::string& codeId, ITimeType timeType, const IKLine& newKline)
{
	IKLine kline;
	return MakeAndGet_Env()->GetDB_KLine()->GetOne(codeId, timeType, newKline.time, kline);

}

bool CCalculator::GetLastBar(const std::string &codeId, ITimeType timeType, const IKLine &newKline, IKLine &lastKline)
{
	IQuery query;
	query.byReqType = 2;
	query.dwSubscribeNum = 1;
	query.tTime = newKline.time - 1;

	IKLines values;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timeType, query, values);
	if (values.empty()) return false;

	lastKline = values[0];
	return true;

}

