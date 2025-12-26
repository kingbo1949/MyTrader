#pragma once
#include "Calculator_MacdDiv.h"
#include "Calculator_Macd.h"
class CCalculator_MacdDiv_Real : public CCalculator_MacdDiv
{
public:
	CCalculator_MacdDiv_Real(Calculator_MacdPtr	macdCalculator);
	virtual ~CCalculator_MacdDiv_Real() { ; };

	virtual void            Initialize(const IBKLinePtrs& klines) override final;

	virtual void			Update(IBKLinePtr newKline, bool isNewData) override final;

	virtual DivergenceType	GetMacdDivAtIndex(size_t index) const  override final;
	virtual DivergenceType	GetMacdDivAtTime(Tick_T time) const  override final;


protected:
	Calculator_MacdPtr					m_macdCalculator;
	IBKLinePtrs							m_klines;
	DivergenceTypes						m_values;
	std::map<Tick_T, DivergenceType>	m_valueMap;

	// 计算指定索引位置的背离属性
	DivergenceType						ScanDivergenceType(const IBKLinePtrs& klines, size_t index);

	// 返回true为top, false是bottom
	std::optional<bool>					CountDivType(const IBKLinePtrs& klines, size_t beginPos, size_t index, const HighAndLow& highLow);

	// 从m_macdCalculator中取数据
	MacdValues							GetMacdValues(size_t beginPos, size_t endPos);

	// 取MacdValues中dif的最大最小值
	HighLowPair							GetHighLow_MacdDif(const MacdValues& values);


};

