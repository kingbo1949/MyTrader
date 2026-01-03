#pragma once
#include "Calculator.h"
#include <optional>
class CCalculator_DivType : public CCalculator
{
public:
	struct HighAndLow
	{
		time_t			highPos = 0;
		std::string		highPosStr = "";
		double			high = -9999999.0;

		time_t			lowPos = 0;
		std::string		lowPosStr = "";
		double			low = 9999999.0;
	};
	enum class MainOrSub
	{
		Main,
		Sub
	};

public:
	CCalculator_DivType();
	virtual ~CCalculator_DivType() { ; };

	virtual void		Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines) override final;

	virtual void		Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline) override final;

protected:
	size_t						m_circle;
	size_t						m_uturnCircle;
	IDivTypeValue				MakeOneInitValue();

	// 计算kline位置的背离属性, klines是kline之前的circle个BAR
	IDivType					ScanDivergenceType(const std::string& codeId, ITimeType timeType, const IKLines& klines, const IKLine& kline);

	// 根据klines[size() - 1]位置的DivType来计算klines[size() - 2]位置的macd是否是反转
	bool						IsUTurn(const std::string& codeId, ITimeType timeType, const IKLines& klines, IDivType divType);

	// 计算kline位置的背离属性, klines是kline之前的circle个BAR
	std::optional<bool>			CountDivType(const std::string& codeId, ITimeType timeType, const IKLines& klines, const IKLine& kline, MainOrSub mainOrSub);


	// 得到klines的高低价
	HighAndLow					MakeHighAndLow(const IKLines& klines, bool useOpenAndClose);

	// 取MacdValues中dif的最大最小值
	HighAndLow					GetHighLow_MacdDif(const IMacdValues& values);

	// 取klines所对应的macd
	IMacdValues					GetMacdValues(const std::string& codeId, ITimeType timeType, const IKLines& klines);
	IMacdValue					GetMacdValue(const std::string& codeId, ITimeType timeType, const IKLine& kline);


	void				UpdateValuesToDb(const std::string& codeId, ITimeType timeType, const IDivTypeValues& values);
	void				UpdateToDb(const std::string& codeId, ITimeType timeType, const IDivTypeValue& value);




};

