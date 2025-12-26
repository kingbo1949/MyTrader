#include "pch.h"
#include "Container_Ma.h"
#include <Factory_IBGlobalShare.h>
#include "Calculator_Ma.h"
CContainer_Ma::CContainer_Ma()
{
	m_calculators[Time_Type::M1] = std::make_shared<CCalculator_Ma>();
	m_calculators[Time_Type::M5] = std::make_shared<CCalculator_Ma>();
	m_calculators[Time_Type::M15] = std::make_shared<CCalculator_Ma>();
	m_calculators[Time_Type::M30] = std::make_shared<CCalculator_Ma>();
	m_calculators[Time_Type::H1] = std::make_shared<CCalculator_Ma>();
	m_calculators[Time_Type::D1] = std::make_shared<CCalculator_Ma>();

}

void CContainer_Ma::Initialize(Time_Type timeType, const IBKLinePtrs& klines)
{
	GetAverageCalculator(timeType)->Initialize(klines);
}

void CContainer_Ma::Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData)
{
	GetAverageCalculator(timeType)->Update(newKline, isNewData);
}


AverageValue CContainer_Ma::GetValueAtIndex(Time_Type timeType, int index)
{
	return GetAverageCalculator(timeType)->GetValueAtIndex(index);
}

AverageValue CContainer_Ma::GetValueAtTime(Time_Type timeType, Tick_T time)
{
	time = CGetRecordNo::GetRecordNo(timeType, time);
	return GetAverageCalculator(timeType)->GetValueAtTime(time);
}
Calculator_AveragePtr CContainer_Ma::GetAverageCalculator(Time_Type timeType)
{
	auto pos = m_calculators.find(timeType);
	if (pos == m_calculators.end())
	{
		printf("can not find ma calculator, time_type = %s \n", CTransToStr::Get_TimeType(timeType).c_str());
		exit(-1);
	}
	return pos->second;
}

