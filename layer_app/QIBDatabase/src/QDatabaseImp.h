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

	virtual void UpdateTickToDB(ITick tick, const ::Ice::Current& current) override final;

	virtual ITick GetLastUpdateTick(int& updateCount, long long int& recentUpdateSecnd, const ::Ice::Current& current) override final;


	// tick表
	virtual void RemoveAllTicks(::std::string codeId, const ::Ice::Current& current) override final;

	virtual void RemoveTicksByRange(::std::string codeId, long long int beginTime, long long int endTime, const ::Ice::Current& current) override final;

	virtual void GetTicks(::std::string codeId, IQuery query, ITicks& ticks, const ::Ice::Current& current) override final;

	virtual bool GetOneTick(::std::string codeId, long long int timePos, ITick& tick, const ::Ice::Current& current) override final;


	// kline表
	virtual void UpdateKLine(::std::string codeId, ITimeType timeType, IKLine kline, const ::Ice::Current& current) override final;

	virtual void RemoveAllKLines(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) override final;

	virtual void RemoveKLinesByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current) override final;

	virtual void RemoveOneKLine(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current) override final;

	virtual void GetKLines(::std::string codeId, ITimeType timeType, IQuery query, IKLines& klines, const ::Ice::Current& current) override final;

	virtual bool GetOneKLine(::std::string codeId, ITimeType timeType, long long int timePos, IKLine& kline, const ::Ice::Current& current) override final;

	virtual void GetKLinePairs(::std::string first, ::std::string second, ITimeType timeType, IQuery query, IKLinePairs& kLinePairs, const ::Ice::Current& current) override final;

	virtual void GetInvalidKLines(::std::string codeId, ITimeType timeType, IKLines& klines, const ::Ice::Current& current) override final;


	// MA表
	virtual void RecountMa(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) override final;

	virtual void RecountMaFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current)  override final;

	virtual void UpdateMa(::std::string codeId, ITimeType timeType, IAvgValue ma, const ::Ice::Current& current)override final;

	virtual void RemoveAllMas(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) override final;

	virtual void RemoveMasByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current) override final;

	virtual void GetMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& mas, const ::Ice::Current& current) override final;

	virtual bool GetOneMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ma, const ::Ice::Current& current) override final;

	// VwMa表

	virtual void RecountVwMa(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) override final;

	virtual void RecountVwMaFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current) override final;

	virtual void UpdateVwMa(::std::string codeId, ITimeType timeType, IAvgValue vwma, const ::Ice::Current& current) override final;

	virtual void RemoveAllVwMas(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) override final;

	virtual void RemoveVwMasByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current) override final;

	virtual void GetVwMas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& vwmas, const ::Ice::Current& current) override final;

	virtual bool GetOneVwMa(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& vwma, const ::Ice::Current& current) override final;


	// Ema表
	virtual void RecountEma(::std::string codeId, ITimeType timeType, const ::Ice::Current& current)  override final;

	virtual void RecountEmaFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current)  override final;

	virtual void UpdateEma(::std::string codeId, ITimeType timeType, IAvgValue ema, const ::Ice::Current& current)  override final;

	virtual void RemoveAllEmas(::std::string codeId, ITimeType timeType, const ::Ice::Current& current)  override final;

	virtual void RemoveEmasByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current)  override final;

	virtual void GetEmas(::std::string codeId, ITimeType timeType, IQuery query, IAvgValues& emas, const ::Ice::Current& current)  override final;

	virtual bool GetOneEma(::std::string codeId, ITimeType timeType, long long int timePos, IAvgValue& ema, const ::Ice::Current& current)  override final;

	// macd表
	virtual void RecountMacd(::std::string codeId, ITimeType timeType, const ::Ice::Current& current)  override final;

	virtual void RecountMacdFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current)  override final;

	virtual void UpdateMacd(::std::string codeId, ITimeType timeType, IMacdValue macd, const ::Ice::Current& current)  override final;

	virtual void RemoveAllMacds(::std::string codeId, ITimeType timeType, const ::Ice::Current& current)  override final;

	virtual void RemoveMacdsByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current)  override final;

	virtual void GetMacds(::std::string codeId, ITimeType timeType, IQuery query, IMacdValues& macds, const ::Ice::Current& current)  override final;

	virtual bool GetOneMacd(::std::string codeId, ITimeType timeType, long long int timePos, IMacdValue& macd, const ::Ice::Current& current)  override final;

	// divtype表
	virtual void RecountDivType(::std::string codeId, ITimeType timeType, const ::Ice::Current& current)  override final;

	virtual void RecountDivTypeFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current)  override final;

	virtual void UpdateDivType(::std::string codeId, ITimeType timeType, IDivTypeValue divtype, const ::Ice::Current& current)  override final;

	virtual void RemoveAllDivTypes(::std::string codeId, ITimeType timeType, const ::Ice::Current& current)  override final;

	virtual void RemoveDivTypesByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current)  override final;

	virtual void GetDivTypes(::std::string codeId, ITimeType timeType, IQuery query, IDivTypeValues& divtypes, const ::Ice::Current& current)  override final;

	virtual bool GetOneDivType(::std::string codeId, ITimeType timeType, long long int timePos, IDivTypeValue& divtype, const ::Ice::Current& current)  override final;

	// AvgAtr表

	virtual void RecountAtr(::std::string codeId, ITimeType timeType, const ::Ice::Current& current)  override final;

	virtual void RecountAtrFromTimePos(::std::string codeId, ITimeType timeType, long long int timePos, const ::Ice::Current& current)  override final;

	virtual void UpdateAtr(::std::string codeId, ITimeType timeType, IAtrValue artValue, const ::Ice::Current& current) final;

	virtual void RemoveAllAtrs(::std::string codeId, ITimeType timeType, const ::Ice::Current& current) final;

	virtual void RemoveAtrsByRange(::std::string codeId, ITimeType timeType, long long int beginTime, long long int endTime, const ::Ice::Current& current) final;

	virtual void GetAtrs(::std::string codeId, ITimeType timeType, IQuery query, IAtrValues& avgAtrs, const ::Ice::Current& current) final;

	virtual bool GetOneAtr(::std::string codeId, ITimeType timeType, long long int timePos, IAtrValue& avgAtr, const ::Ice::Current& current) final;






protected:
	Int				m_updateCount;
	Long			m_recentUpdateSecnd;
	ITick			m_recentUpdateTick;
	TickUpdatorPtr	m_pTickUpdator;

	void		RecountMa(::std::string codeId, ITimeType timetype);
	void		RecountVwMa(::std::string codeId, ITimeType timetype);
	void		RecountEma(::std::string codeId, ITimeType timetype);
	void		RecountMacd(::std::string codeId, ITimeType timetype);
	void		RecountDivType(::std::string codeId, ITimeType timetype);
	void		RecountAtr(::std::string codeId, ITimeType timetype);

	void		RecountMaFromTimePos(const ::std::string& codeId, ITimeType timetype, long long int timePos);
	void		RecountVwMaFromTimePos(const ::std::string& codeId, ITimeType timetype, long long int timePos);
	void		RecountEmaFromTimePos(const ::std::string& codeId, ITimeType timetype, long long int timePos);
	void		RecountMacdFromTimePos(const ::std::string& codeId, ITimeType timetype, long long int timePos);
	void		RecountDivTypeFromTimePos(const ::std::string& codeId, ITimeType timetype, long long int timePos);
	void		RecountAtrFromTimePos(const ::std::string& codeId, ITimeType timetype, long long int timePos);

	void 		GetKline_RecountQuery_All(const std::string& codeId, ITimeType timetype, IKLines& klines);
	void		GetKline_RecountQuery_TimePos(const std::string& codeId, ITimeType timetype, long long int timePos, IKLines& klines);
};

typedef std::shared_ptr<CQDatabaseImp> QDatabaseImpPtr;