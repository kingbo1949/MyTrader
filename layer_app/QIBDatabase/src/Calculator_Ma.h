#pragma once
#include "Calculator_Average.h"
class CCalculator_Ma : public CCalculator_Average
{
public:
	CCalculator_Ma() { ; };
	virtual ~CCalculator_Ma() { ; };

	virtual void 		Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines) override final;

	virtual void		Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline) override final;

protected:
	// 一共有5 20 60 200四种circle，根据circle填写values中所有对应circle的值
	void				Initialize(const IKLines& klines, int circle, IAvgValues& values);

	// 计算末尾circle根k线均值
	double				CountAverage(const std::string& codeId, ITimeType timeType, const IKLine& newKline, int circle);

	virtual void		UpdateToDb(const std::string& codeId, ITimeType timeType, const IAvgValue& value) override final;


};

