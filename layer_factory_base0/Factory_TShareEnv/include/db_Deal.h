#pragma once

#include <base_trade.h>
class Cdb_Deal
{
public:
	Cdb_Deal() { ; };
	virtual ~Cdb_Deal() { ; };

	virtual FileStatus		GetStatus() = 0;

	virtual void			SetStatus(FileStatus fileStatus) = 0;

	virtual void			AddOne(OneDealPtr pdeal) = 0;

	virtual OneDealPtr		GetOne(const DealKey& key, int dealNo) = 0;

	virtual OneDealPtrs		GetAll(const DealKey& key) = 0;

	virtual OneDealPtrs		GetAll() = 0;


};

typedef std::shared_ptr<Cdb_Deal> db_DealPtr;