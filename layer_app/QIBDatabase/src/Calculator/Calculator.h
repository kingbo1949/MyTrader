#pragma once
#include "../GlobalDefine.h"
class CCalculator
{
public:
	CCalculator() { ; };
	virtual ~CCalculator() { ; };

	// 初始化时用所有K线，计算出所有指标数据，然后更新到数据库
	virtual void		Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines) = 0;

	// 盘中实时生成了K线之后，开始更新指标数据
	virtual void		Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline) = 0;

	// 检查传入的K线是否已经存在于数据库
	bool				Exist(const std::string& codeId, ITimeType timeType, const IKLine& newKline);

	// 按传入的K线返回其上一根K线
	bool				GetLastBar(const std::string& codeId, ITimeType timeType, const IKLine& newKline, IKLine& lastKline);

};
typedef std::shared_ptr<CCalculator> CalculatorPtr;


CalculatorPtr MakenAndGet_Calculator_Ma();
CalculatorPtr MakenAndGet_Calculator_VwMa();
CalculatorPtr MakenAndGet_Calculator_Ema();
CalculatorPtr MakenAndGet_Calculator_Macd();
CalculatorPtr MakenAndGet_Calculator_DivType();
CalculatorPtr MakenAndGet_Calculator_Atr();