#pragma once
#include "db_Deal.h"
class Cdb_Deal_Real : public Cdb_Deal
{
public:
	Cdb_Deal_Real():m_status(FileStatus::Saved){ ; };
	virtual ~Cdb_Deal_Real() { ; };

	virtual FileStatus		GetStatus() override final;

	virtual void			SetStatus(FileStatus fileStatus) override final;

	virtual void			AddOne(OneDealPtr pdeal) override final;

	virtual OneDealPtr		GetOne(const DealKey& key, int dealNo) override final;

	virtual OneDealPtrs		GetAll(const DealKey& key) override final;

	virtual OneDealPtrs		GetAll() override final;

protected:
	std::mutex						m_mutex;
	std::map<DealKey, OneDealPtrs>	m_deals;
	FileStatus						m_status;



};

