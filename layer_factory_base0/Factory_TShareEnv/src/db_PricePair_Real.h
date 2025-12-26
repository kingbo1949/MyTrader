#pragma once
#include "db_PricePair.h"
class Cdb_PricePair_Real : public Cdb_PricePair
{
public:
	Cdb_PricePair_Real();
	virtual ~Cdb_PricePair_Real() { ; };

	virtual FileStatus						GetStatus() override final ;

	virtual void							SetStatus(FileStatus fileStatus)  override final;


	virtual PricePairPtr					GetPricePair(StrategyIdHashId strategyIdHashId) override final;

	virtual void							Clear(StrategyIdHashId strategyIdHashId) override final;

	virtual void							GetAll(std::map<StrategyIdHashId, PricePairPtr>& pricePairs) override final;

	virtual void							PrintAll() override final;

protected:
	std::mutex									m_mutex;
	std::map<StrategyIdHashId, PricePairPtr>	m_pricePairs;
	FileStatus									m_status;


};

