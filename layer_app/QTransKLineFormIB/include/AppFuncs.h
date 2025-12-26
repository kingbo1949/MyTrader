#pragma once

#include <base_struc.h>

// 本模块使用的全局函数
class CAppFuncs
{
public:
	CAppFuncs() { ; };
	virtual ~CAppFuncs() { ; };

	// 检查klines, 如果其中有K线，高点与低点是相同的价格，则返回false
	static bool		CheckKLines(IbContractPtr contract, const IBKLinePtrs& klines);

	// dayKLine中的高低价必须在klines中出现过
	static bool		CheckKLines(IbContractPtr contract, Time_Type timeType, const IBKLinePtrs& klines, IBKLinePtr dayKLine);

	// 当dayKLine高低价没有出现在klinesHighLow中，需要打印错误
	static void		PrintErr_HighLow_Klines(Time_Type timeType, const HighAndLow& klinesHighLow, IBKLinePtr dayKLine);

	static void		UpdateToDb(const std::string& codeId, Time_Type timeType, const IBKLinePtrs& klines);

	// 删除指定时间段的k线和指标
	static void		DelToDb(const std::string& codeId, Time_Type timeType, const TimePair& timePair);


};

