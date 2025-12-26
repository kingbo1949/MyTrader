#pragma once

#include <base_trade.h>
class Cdb_PricePair
{
public:
	Cdb_PricePair() { ; };
	virtual ~Cdb_PricePair() { ; };

	virtual FileStatus				GetStatus() = 0;

	virtual void					SetStatus(FileStatus fileStatus) = 0;

	virtual PricePairPtr			GetPricePair(StrategyIdHashId strategyIdHashId) = 0;

	virtual void					Clear(StrategyIdHashId strategyIdHashId) = 0;

	virtual void					GetAll(std::map<StrategyIdHashId, PricePairPtr>& pricePairs) = 0;

	virtual void					PrintAll() = 0;
};
typedef std::shared_ptr<Cdb_PricePair> db_PricePairPtr;

