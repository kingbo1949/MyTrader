#pragma once
#include <base_struc.h>
// 重建K线的指标群
class CRecountKline
{
public:
	CRecountKline() { ; };
	virtual ~CRecountKline() { ; };

	void	Go();
protected:
	void	RecountKLine(const CodeStr& codeId);
	void	RecountKLine_Ma(const CodeStr& codeId);
	void	RecountKLine_Ema(const CodeStr& codeId);
	void	RecountKLine_Macd(const CodeStr& codeId);
	void	RecountKLine_DivType(const CodeStr& codeId);


};

