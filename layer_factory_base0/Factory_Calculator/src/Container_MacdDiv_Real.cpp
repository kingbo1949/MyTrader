#include "pch.h"
#include "Container_MacdDiv_Real.h"
#include "Calculator_MacdDiv_Real.h"
#include <Factory_IBGlobalShare.h>
CContainer_MacdDiv_Real::CContainer_MacdDiv_Real(Container_MacdPtr pContainerMacd)
{
	m_calculators[Time_Type::M1] = std::make_shared<CCalculator_MacdDiv_Real>(pContainerMacd->GetMacdCalculator(Time_Type::M1));
	m_calculators[Time_Type::M5] = std::make_shared<CCalculator_MacdDiv_Real>(pContainerMacd->GetMacdCalculator(Time_Type::M5));
	m_calculators[Time_Type::M15] = std::make_shared<CCalculator_MacdDiv_Real>(pContainerMacd->GetMacdCalculator(Time_Type::M15));
	m_calculators[Time_Type::M30] = std::make_shared<CCalculator_MacdDiv_Real>(pContainerMacd->GetMacdCalculator(Time_Type::M30));
	m_calculators[Time_Type::H1] = std::make_shared<CCalculator_MacdDiv_Real>(pContainerMacd->GetMacdCalculator(Time_Type::H1));
	m_calculators[Time_Type::D1] = std::make_shared<CCalculator_MacdDiv_Real>(pContainerMacd->GetMacdCalculator(Time_Type::D1));

}

void CContainer_MacdDiv_Real::Initialize(Time_Type timeType, const IBKLinePtrs& klines)
{
	GetMacdDivCalculator(timeType)->Initialize(klines);
}

void CContainer_MacdDiv_Real::Update(Time_Type timeType, IBKLinePtr newKline, bool isNewData)
{
	GetMacdDivCalculator(timeType)->Update(newKline, isNewData);
}

DivergenceType CContainer_MacdDiv_Real::GetMacdDivAtIndex(Time_Type timeType, size_t index) const
{
	return GetMacdDivCalculator(timeType)->GetMacdDivAtIndex(index);
}

DivergenceType CContainer_MacdDiv_Real::GetMacdDivAtTime(Time_Type timeType, Tick_T time) const
{
	time = CGetRecordNo::GetRecordNo(timeType, time);
	return GetMacdDivCalculator(timeType)->GetMacdDivAtTime(time);
}

Calculator_MacdDivPtr CContainer_MacdDiv_Real::GetMacdDivCalculator(Time_Type timeType) const
{
	auto pos = m_calculators.find(timeType);
	if (pos == m_calculators.end())
	{
		printf("can not find macdDiv calculator, time_type = %s \n", CTransToStr::Get_TimeType(timeType).c_str());
		exit(-1);
	}
	return pos->second;
}
