#include "pch.h"
#include "Container_Macd_Real.h"
#include <Factory_IBGlobalShare.h>
#include "Calculator_Macd_Real.h"
CContainer_Macd_Real::CContainer_Macd_Real()
{
	m_calculators[Time_Type::M1] = std::make_shared<CCalculator_Macd_Real>();
	m_calculators[Time_Type::M5] = std::make_shared<CCalculator_Macd_Real>();
	m_calculators[Time_Type::M15] = std::make_shared<CCalculator_Macd_Real>();
	m_calculators[Time_Type::M30] = std::make_shared<CCalculator_Macd_Real>();
	m_calculators[Time_Type::H1] = std::make_shared<CCalculator_Macd_Real>();
	m_calculators[Time_Type::D1] = std::make_shared<CCalculator_Macd_Real>();

}

void CContainer_Macd_Real::Initialize(Time_Type timeType, const IBKLinePtrs& klines)
{
	GetMacdCalculator(timeType)->Initialize(klines);
}

void CContainer_Macd_Real::Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData)
{
	GetMacdCalculator(timeType)->Update(newKline, isNewData);
}

MacdValue CContainer_Macd_Real::GetMacdAtIndex(Time_Type timeType, int index)
{
	return GetMacdCalculator(timeType)->GetMacdAtIndex(index);
}

MacdValue CContainer_Macd_Real::GetMacdAtTime(Time_Type timeType, Tick_T time)
{
	time = CGetRecordNo::GetRecordNo(timeType, time);
	return GetMacdCalculator(timeType)->GetMacdAtTime(time);
}

Calculator_MacdPtr CContainer_Macd_Real::GetMacdCalculator(Time_Type timeType)
{
	auto pos = m_calculators.find(timeType);
	if (pos == m_calculators.end())
	{
		printf("can not find macd calculator, time_type = %s \n", CTransToStr::Get_TimeType(timeType).c_str());
		exit(-1);
	}
	return pos->second;
}

