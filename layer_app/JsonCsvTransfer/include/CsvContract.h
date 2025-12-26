#pragma once
#include "CsvTransor.h"
#include <base_struc.h>
class CCsvContract : public CCsvTransor
{
public:
	CCsvContract() { ; };
	virtual ~CCsvContract() { ; };

	// 从csv文件读取StrategyTotalParamExs
	virtual void				LoadContractFromCsv(const std::string& fileName, IbContractPtrs& contracts) = 0;

};
typedef std::shared_ptr<CCsvContract> CsvContractPtr;
