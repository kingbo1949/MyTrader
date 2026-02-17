#pragma once
#include <QDatabase.h>
#include "TickUpdator.h"
using namespace IBTrader;
using namespace Ice;
class CQDatabaseImp : public IQDatabase
{
public:
	CQDatabaseImp();
	virtual ~CQDatabaseImp() { ; };

	virtual int IdlCount(const ::Ice::Current& current) final;

	virtual bool IsAllIdle(const ::Ice::Current& current) final;

	virtual int TaskCount(const ::Ice::Current& current) final;


	virtual void UpdateTickToDB(ITick tick, const ::Ice::Current& current) override final;

	virtual ITick GetLastUpdateTick(int& updateCount, long long int& recentUpdateSecnd, const ::Ice::Current& current) override final;


	// tick表
	virtual void RemoveAllTicks(::std::string codeId, const ::Ice::Current& current) override final;

	virtual void RemoveTicksByRange(::std::string codeId, long long int beginTime, long long int endTime, const ::Ice::Current& current) override final;

	virtual void GetTicks(::std::string codeId, IQuery query, ITicks& ticks, const ::Ice::Current& current) override final;

	virtual bool GetOneTick(::std::string codeId, long long int timePos, ITick& tick, const ::Ice::Current& current) override final;


	// kline表
	virtual void UpdateKLine(::std::string codeId, ITimeType timeType, IKLine kline, const ::Ice::Current& current) override final;

	virtual void UpdateKLines(::std::string codeId, ITimeType timeType, IKLines klines, const ::Ice::Current& current) final;

	virtual void RemoveAllKLines(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) override final;

	virtual void RemoveKLinesByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current) override final;

	virtual void RemoveOneKLine(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current) override final;

	virtual void GetKLines(::std::string codeId, ITimeType timeType, IQuery query, IKLines& klines, const ::Ice::Current& current) override final;

	virtual bool GetOneKLine(::std::string codeId, ITimeType timeType, long long int timePos, IKLine& kline, const ::Ice::Current& current) override final;

	virtual void GetKLinePairs(::std::string first, ::std::string second, ITimeType timeType, IQuery query, IKLinePairs& kLinePairs, const ::Ice::Current& current) override final;

	virtual void GetInvalidKLines(::std::string codeId, ITimeType timeType, IKLines& klines, const ::Ice::Current& current) override final;

	// 指标更新
	virtual void RecountAllIndex(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) final;
	virtual void UpdateAllIndexFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current) final;



	// MA表
	virtual void GetMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& mas, const ::Ice::Current& current) override final;

	virtual bool GetOneMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ma, const ::Ice::Current& current) override final;

	// VwMa表

	virtual void GetVwMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& vwmas, const ::Ice::Current& current) override final;

	virtual bool GetOneVwMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& vwma, const ::Ice::Current& current) override final;


	// Ema表
	virtual void GetEmas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& emas, const ::Ice::Current& current)  override final;

	virtual bool GetOneEma(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ema, const ::Ice::Current& current)  override final;

	// macd表
	virtual void GetMacds(::std::string codeId, ITimeType timeType, IQuery query, IMacdValues& macds, const ::Ice::Current& current)  override final;

	virtual bool GetOneMacd(::std::string codeId, ITimeType timeType, long long int timePos, IMacdValue& macd, const ::Ice::Current& current)  override final;

	// divtype表
	virtual void GetDivTypes(::std::string codeId, ITimeType timeType, IQuery query, IDivTypeValues& divtypes, const ::Ice::Current& current)  override final;

	virtual bool GetOneDivType(::std::string codeId, ITimeType timeType, long long int timePos, IDivTypeValue& divtype, const ::Ice::Current& current)  override final;

	// AvgAtr表
	virtual void GetAtrs(::std::string codeId, ITimeType timeType, IQuery query, IAtrValues& avgAtrs, const ::Ice::Current& current) final;

	virtual bool GetOneAtr(::std::string codeId, ITimeType timeType, long long int timePos, IAtrValue& avgAtr, const ::Ice::Current& current) final;






protected:
	Int				m_updateCount;
	Long			m_recentUpdateSecnd;
	ITick			m_recentUpdateTick;
	TickUpdatorPtr	m_pTickUpdator;

};

typedef std::shared_ptr<CQDatabaseImp> QDatabaseImpPtr;