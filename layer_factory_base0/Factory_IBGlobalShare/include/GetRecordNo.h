#pragma once
#include "Factory_IBGlobalShare_Export.h"
#include <base_struc.h>
class FACTORY_IBGLOBALSHARE_API CGetRecordNo
{
public:
	CGetRecordNo() { ; };
	virtual ~CGetRecordNo() { ; };


	// 计算tick所在的时间标头，返回值单位毫秒
	static Tick_T	GetRecordNo(Time_Type timeType, Tick_T tickTime);

	// 计算tick所在的时间标尾，返回值单位毫秒
	static Tick_T	GetRecordNoEnd(Time_Type timeType, Tick_T tickTime);

	// 指定时间类型包含的时间跨度 毫秒
	static Tick_T	GetTimeSpan(Time_Type timeType);

	// 计算tick所在的时间标头，返回值单位毫秒
	static Tick_T	GetRecordNo_Month(Tick_T tickTime);

	// 计算tick所在的时间标尾，返回值单位毫秒
	static Tick_T	GetRecordNoEnd_Month(Tick_T tickTime);

	// dayTimeMs本意是一个零点毫秒，因为有可能是不同的夏时制，需要转换统一起来
	static Tick_T	ModifyDayTimeMs(Tick_T dayTimeMs);

	// 判断指定时间是否位于指定Kline内
	static bool		TimeInKLine(Time_Type timeType, IBKLinePtr kline, Tick_T time);

	// 判断两个时间是否位于相同的bar上
	static bool		TimesInSameBar(Time_Type timeType, Tick_T time1, Tick_T time2);





protected:

	static Tick_T	GetRecordNo_S15(Tick_T tickTime);

	static Tick_T	GetRecordNo_M1(Tick_T tickTime);

	static Tick_T	GetRecordNo_M5(Tick_T tickTime);

	static Tick_T	GetRecordNo_M15(Tick_T tickTime);

	static Tick_T	GetRecordNo_M30(Tick_T tickTime);

	static Tick_T	GetRecordNo_H1(Tick_T tickTime);

	static Tick_T	GetRecordNo_D1(Tick_T tickTime);

	static bool		Trans(Tick_T tickTime, tm& tt);



};

