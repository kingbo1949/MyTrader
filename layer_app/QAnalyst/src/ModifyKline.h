#pragma once
#include <base_struc.h>
#include <set>
#include "TradeDay.h"
// 深度学习需要每天的K线数量一样多，这个类负责处理K线 把每天的数据弄成一样多
class CModifyKline
{
public:
	CModifyKline(const CodeStr& codeId, Time_Type timeType, IBKLinePtrs klines);
	virtual ~CModifyKline() { ; }


	// 一般交易日30分钟线最后一根的时间是15:30:00 把不满足条件的输出到文件 然后把数据补齐
	void							CheckHoliday();

	// 一般交易日30分钟线有46根， 把不满足根数数据的日期输出到文件
	void							CheckDayKLineCount();

	// 删除所有成交量为0的记录
	void							CheckVol();


	// 有些数据需要手动添加
	void							AddSpecaildata();

protected:
	TradeDayPtr						m_pTradeDay;
	CodeStr							m_codeId;
	Time_Type						m_timeType;
	IBKLinePtrs						m_klines;

	// 每个交易日需要结束在正确的时间 把非常结束的交易日找出来
	std::vector<Tick_T>				NeedEnd();

	// 指定交易日没有正常结束 把它的K线补全
	void							PadingDay(Tick_T daytime);

};

