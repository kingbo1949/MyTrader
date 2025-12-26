#pragma once
#include "CsvContract.h"
class CCsvContract_Real : public CCsvContract
{
public:
	CCsvContract_Real() { ; };
	virtual ~CCsvContract_Real() { ; };

	// 从csv文件读取StrategyTotalParamExs
	virtual void				LoadContractFromCsv(const std::string& fileName, IbContractPtrs& contracts) override final;

protected:
	// csv字符串转换成结构
	IbContractPtr				MakeContract(const std::string& csvStr);


};

