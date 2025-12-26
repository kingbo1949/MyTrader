#pragma once
#include <base_struc.h>
#include <set>
#include "TradeDay.h"
class CSaveData_Klines
{
public:
	CSaveData_Klines(const CodeStr& codeId, IBKLinePtrs klines);
	virtual ~CSaveData_Klines() { ; }


	void							Go(const std::string& fileName);


protected:
	IBKLinePtrs						m_klines;
	CodeStr							m_codeId;
	TradeDayPtr						m_pTradeDay;


	// 得到表头
	std::string						GetTitle();

	std::string						GetKlineStr(IBKLinePtr kline);



};

