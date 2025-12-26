#pragma once
#include "AnalystDealDefine.h"
class CAnalystResult_Real : public CAnalystResult
{
public:
	CAnalystResult_Real() { ; };
	virtual ~CAnalystResult_Real() { ; };

	virtual	void			Print_Simple() override final;		// 简略打印


	virtual	void			PrintToFile(const std::string& fileName, const SimpleMatchPtrs& matchs, bool needDealTime) override final;

protected:
	// 连续亏损次数和其时间区间 字符串
	std::string				GetSeriesLoss_Simple();

	std::string				GetFirstTwo(WinOrLoss winOrLoss, const SimpleMatchPtrs& matchs);

	// match字符串 盈利 开仓时间->平仓时间
	std::string				GetMatchStr(SimpleMatchPtr match);

	std::string				GetMatchStr_Csv(SimpleMatchPtr match, bool needDealTime);

};

