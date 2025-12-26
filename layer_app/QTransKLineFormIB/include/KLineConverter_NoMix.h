#pragma once

#include <base_struc.h>
class CKLineConverter_NoMix
{
public:
	CKLineConverter_NoMix() { ; };
	virtual ~CKLineConverter_NoMix() { ; };

	void				ConvertOneKLineFromIBToDb(const CodeStr& codeId, TimePair timePair);

	void				QueryKLineInDb(const CodeStr& codeId);

	void				RequestKLineInDb(const CodeStr& codeId, Time_Type timeType, time_t beginPos, time_t endPos);

	IBKLinePtrs			QueryFromIb(const CodeStr& codeId, Time_Type timeType, TimePair timePair);

protected:
	void				QueryKLineInDb(const CodeStr& codeId, Time_Type timeType);

	void				ConvertOneKLineFromIBToDb(const CodeStr& codeId, Time_Type timeType, TimePair timePair);



	// 得到IB查询的倒叙天数
	int					GetDayForIbQuery(Time_Type timeType);




};

