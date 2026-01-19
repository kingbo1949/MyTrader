#pragma once
#include "stdafx.h"
#include <set>
#include "TradeDay.h"
class CSaveData_Klines
{
public:
	CSaveData_Klines(const CodeStr& codeId, const KLine4Tables& kline4Tables);
	virtual ~CSaveData_Klines() { ; }


	void							Go(const std::string& fileName);


protected:
	KLine4Tables					m_kline4Tables;
	CodeStr							m_codeId;
	TradeDayPtr						m_pTradeDay;


	// 得到表头
	std::string						GetTitle();

	std::string						GetKlineStr(const KLine4Table &kLine4Table);



};

