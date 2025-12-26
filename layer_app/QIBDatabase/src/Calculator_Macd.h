#pragma once
#include "Calculator.h"
class CCalculator_Macd : public CCalculator
{
public:
	CCalculator_Macd();
	virtual ~CCalculator_Macd() { ; };

	virtual void		Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines) override final;

	virtual void 		Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline) override final;

protected:
	int						m_shortPeriod;
	int						m_longPeriod;
	int						m_signalPeriod;
	double					m_alphaShort;
	double					m_alphaLong;
	double					m_alphaSignal;

	IMacdValue				MakeOneInitValue();

	// 取数据库中newKline之前的值
	IMacdValue				GetLastValue(const std::string& codeId, ITimeType timeType, const IKLine& newKline);


	void				UpdateValuesToDb(const std::string& codeId, ITimeType timeType, const IMacdValues& values);
	void				UpdateToDb(const std::string& codeId, ITimeType timeType, const IMacdValue& value);




};

