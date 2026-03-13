#pragma once
#include "stdafx.h"
#include <set>
#include "TradeDay.h"
class CSaveData_Klines
{
public:
	CSaveData_Klines(const CodeStr& codeId, const IBRichDataPtrs& richDatas);
	virtual ~CSaveData_Klines() { ; }


	void							Go(const std::string& fileName);


protected:
	IBRichDataPtrs					m_richDatas;
	CodeStr							m_codeId;



	// 得到表头
	std::string						GetTitle();

	std::string						GetRichStr(const IBRichDataPtr rich);



};

