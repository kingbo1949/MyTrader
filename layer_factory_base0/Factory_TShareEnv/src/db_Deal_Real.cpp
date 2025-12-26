#include "pch.h"
#include "db_Deal_Real.h"

FileStatus Cdb_Deal_Real::GetStatus()
{
	return m_status;
}

void Cdb_Deal_Real::SetStatus(FileStatus fileStatus)
{
	m_status = fileStatus;
}

void Cdb_Deal_Real::AddOne(OneDealPtr pdeal)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_status = FileStatus::Updated;

	std::map<DealKey, OneDealPtrs>::iterator pos = m_deals.find(pdeal->key);

	if (pos != m_deals.end())
	{
		pos->second.insert(pdeal);
	}
	else
	{
		OneDealPtrs values;
		values.insert(pdeal);
		m_deals.insert(std::pair<DealKey, OneDealPtrs>(pdeal->key, values));
	}
	return;
}

OneDealPtr Cdb_Deal_Real::GetOne(const DealKey& key, int dealNo)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	OneDealPtr targetDeal = std::make_shared<COneDeal>();
	targetDeal->key = key;
	targetDeal->dealNo = dealNo;

	std::map<DealKey, OneDealPtrs>::iterator pos = m_deals.find(key);
	if (pos == m_deals.end()) return nullptr;

	OneDealPtrs::iterator pset = pos->second.find(targetDeal);
	if (pset == pos->second.end()) return nullptr;

	return *pset;

}

OneDealPtrs Cdb_Deal_Real::GetAll(const DealKey& key)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	OneDealPtrs back;
	std::map<DealKey, OneDealPtrs>::iterator pos = m_deals.find(key);
	if (pos == m_deals.end()) return back;

	return pos->second;

}

OneDealPtrs Cdb_Deal_Real::GetAll()
{
	OneDealPtrs back;

	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto oneitem : m_deals)
	{
		for (auto onedeal : oneitem.second)
		{
			back.insert(onedeal);
		}
	}
	return back;
}
