#pragma once
#include "MyQDatabase.h"
class CMyQDatabase_Real : public CMyQDatabase
{
public:
	CMyQDatabase_Real() { ; };
	virtual ~CMyQDatabase_Real() { ; };

	// 更新行情数据库
	virtual void			UpdateTicks(IBTickPtr tick) override final;

	// 查询最近更新到行情数据库的情况
	virtual IBTickPtr		GetLastUpdateTick(int& updateCount, time_t& recentUpdateSecnd)  override final;


	// 取单品种数据
	virtual IBTickPtrs		GetTickHis(const CodeStr& codeId, const QQuery& query)  override final;

	// 取单品种数据 使用接力方式
	virtual IBTickPtrs		GetTickHisByLoop(const CodeStr& codeId, const TimePair& timePair) override final;


	// 取行情报价
	virtual IBTickPtr		GetMarketQuoteDL(const CodeStr& codeId)  override final;

	// 删除单品种所有tick数据
	virtual void			RemoveAllTick(const CodeStr& codeId) override final;

	// 查询单个指定数据
	virtual IBTickPtr		GetOneTick(const CodeStr& codeId, time_t timePos) override final;

	// tick数据太密集，需要过滤tick数据
	// 正常情况按ms毫秒一个TICK来过滤，除非出现超过bigTick价位的跃迁TICK
	virtual IBTickPtrs		FliteTicks(const IBTickPtrs& ticks, time_t ms, int bigTick) override final;


	// --------------- kline 表 ---------------------
	virtual void			UpdateKLine(const CodeStr& codeId, Time_Type timeType, IBKLinePtr kline) override final;

	// 不算当前K线，计算ma的值
	virtual double			MakeMa(const CodeStr& codeId, Time_Type timeType, int circle, Tick_T currentTime) override final;


	// 取单品种数据
	virtual IBKLinePtrs		GetKLine(const CodeStr& codeId, Time_Type timeType, const QQuery& query) override final;

	// 取单品种数据 使用接力方式
	virtual IBKLinePtrs		GetKLineByLoop(const CodeStr& codeId, Time_Type timeType, const TimePair& timePair) override final;


	// 删除单品种所有tick数据
	virtual void			RemoveAllKLine(const CodeStr& codeId, Time_Type timeType) override final;

	// 删除单品种区间数据 区间为[beginTime,endTime]
	virtual void			RemoveKLines(const CodeStr& codeId, Time_Type timeType, const TimePair& timePair) override final;

	// 删除指定记录
	virtual void			RemoveOneKLine(const CodeStr& codeId, Time_Type timeType, time_t timePos) override final;

	// 查询单个指定数据
	virtual IBKLinePtr		GetOneKLine(const CodeStr& codeId, Time_Type timeType, time_t timePos) override final;

	// 取品种对
	virtual IBKLinePairs	GetKLinePair(const CodeStr& firstcode, const CodeStr& secondcode, Time_Type timeType, const QQuery& query) override final;

	// 取无效的BAR
	virtual IBKLinePtrs		GetInvalidKLines(const CodeStr& codeId, Time_Type timeType) override final;



	// --------------- Ma 表 ---------------------
	// 重新计算所有MA
	virtual void			RecountMa(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RecountMaFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime) override final;

	// 取单品种数据
	virtual IBMaPtrs		GetMas(const CodeStr& codeId, Time_Type timeType, const QQuery& query) override final;

	// 删除单品种所有tick数据
	virtual void			RemoveAllMas(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RemoveMasByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime) override final;

	// 查询单个指定数据
	virtual IBMaPtr			GetOneMa(const CodeStr& codeId, Time_Type timeType, time_t timePos) override final;

	// --------------- VwMa 表 ---------------------
	// 重新计算所有MA
	virtual void			RecountVwMa(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RecountVwMaFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime) override final;

	// 取单品种数据
	virtual IBVwMaPtrs		GetVwMas(const CodeStr& codeId, Time_Type timeType, const QQuery& query) override final;

	// 删除单品种所有tick数据
	virtual void			RemoveAllVwMas(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RemoveVwMasByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime) override final;

	// 查询单个指定数据
	virtual IBVwMaPtr		GetOneVwMa(const CodeStr& codeId, Time_Type timeType, time_t timePos) override final;


	// --------------- Ema 表 ---------------------
	// 重新计算所有MA
	virtual void			RecountEma(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RecountEmaFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime) override final;

	// 取单品种数据
	virtual IBEmaPtrs		GetEmas(const CodeStr& codeId, Time_Type timeType, const QQuery& query)  override final;

	// 删除单品种所有tick数据
	virtual void			RemoveAllEmas(const CodeStr& codeId, Time_Type timeType)  override final;

	virtual void			RemoveEmasByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime)  override final;

	// 查询单个指定数据
	virtual IBEmaPtr		GetOneEma(const CodeStr& codeId, Time_Type timeType, time_t timePos) override final;

	// --------------- macd 表 ---------------------
	// 重新计算所有MA
	virtual void			RecountMacd(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RecountMacdFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime) override final;

	// 取单品种数据
	virtual IBMacdPtrs		GetMacds(const CodeStr& codeId, Time_Type timeType, const QQuery& query) override final;

	// 删除单品种所有tick数据
	virtual void			RemoveAllMacds(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RemoveMacdsByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime) override final;

	// 查询单个指定数据
	virtual IBMacdPtr		GetOneMacd(const CodeStr& codeId, Time_Type timeType, time_t timePos) override final;

	// --------------- divtype 表 ---------------------
	// 重新计算所有MA
	virtual void			RecountDivType(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RecountDivTypeFromTimePos(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime) override final;

	// 取单品种数据
	virtual IBDivTypePtrs	GetDivTypes(const CodeStr& codeId, Time_Type timeType, const QQuery& query) override final;

	// 删除单品种所有tick数据
	virtual void			RemoveAllDivTypes(const CodeStr& codeId, Time_Type timeType) override final;

	virtual void			RemoveDivTypesByRange(const CodeStr& codeId, Time_Type timeType, Tick_T beginTime, Tick_T endTime) override final;

	// 查询单个指定数据
	virtual IBDivTypePtr	GetOneDivType(const CodeStr& codeId, Time_Type timeType, time_t timePos) override final;


protected:

	// 是否符合过滤需要添加的条件
	bool					ValidFlite(time_t ms, int bigTick, IBTickPtr thisTick, IBTickPtr lastTick, IBTickPtr lastAddedTick);








};

