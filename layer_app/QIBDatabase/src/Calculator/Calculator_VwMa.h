#pragma once

// 成交量加权均线
#include "Calculator_Average.h"
class CCalculator_VwMa : public CCalculator_Average
{
public:
	CCalculator_VwMa() { ; };
	virtual ~CCalculator_VwMa() { ; };

	virtual void 		Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines) override final;

	virtual void		Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline) override final;

protected:
	// һ����5 20 60 200����circle������circle��дvalues�����ж�Ӧcircle��ֵ
	void				Initialize(const IKLines& klines, int circle, IAvgValues& values);

	// ����ĩβcircle��k�߾�ֵ
	double				CountAverage(const std::string& codeId, ITimeType timeType, const IKLine& newKline, int circle);

	virtual void		UpdateToDb(const std::string& codeId, ITimeType timeType, const IAvgValue& value) override final;

};

