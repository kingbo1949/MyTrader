#pragma once
#include "Calculator.h"
class CCalculator_Average : public CCalculator
{
public:
	CCalculator_Average() { ; };
	virtual ~CCalculator_Average() { ; };


protected:

	// 取得对应circle在value中的引用
	double& GetCircleValue(IAvgValue& value, int circle)
	{
		if (circle == 5) return value.v5;
		if (circle == 20) return value.v20;
		if (circle == 60) return value.v60;
		if (circle == 200) return value.v200;

		printf("error circle, circle = %d \n", circle);
		exit(-1);
		return value.v5;

	}

	void				UpdateValuesToDb(const std::string& codeId, ITimeType timeType, const IAvgValues& values);
	virtual void		UpdateToDb(const std::string& codeId, ITimeType timeType, const IAvgValue& value) = 0;



};


