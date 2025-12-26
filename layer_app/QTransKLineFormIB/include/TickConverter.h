#pragma once

#include <base_struc.h>
class CTickConverter
{
public:
	CTickConverter() { ; };
	virtual ~CTickConverter() { ; };

	// 从IB接口查询盘后数据，到数据库
	void				ConvertTicksFromIBToDb(const CodeStr& codeId, TimePair timePair);

	// 打印tick数目
	void				QueryTickInDb(const CodeStr& codeId, TimePair timePair);

protected:

	void				ConvertTicksFromIBToDb(const CodeStr& codeId, time_t daySecond);

	// 打印tick数目
	void				QueryTickInDb(const CodeStr& codeId, time_t daySecond, MarketType marketType);

	// 打印所有tick
	void				RequestKLineInDb(const CodeStr& codeId, time_t daySecond, MarketType marketType);



};

