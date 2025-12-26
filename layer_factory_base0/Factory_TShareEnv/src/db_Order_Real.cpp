#include "pch.h"
#include "db_Order_Real.h"
#include <TransToStr.h>
Cdb_Order_Real::Cdb_Order_Real(ActiveOrInactive activeOrInactive)
	:m_activeOrInactive(activeOrInactive), m_status(FileStatus::Saved)
{

}

FileStatus Cdb_Order_Real::GetStatus()
{
	return m_status;
}

void Cdb_Order_Real::SetStatus(FileStatus fileStatus)
{
	m_status = fileStatus;
}

void Cdb_Order_Real::AddOne(OneOrderPtr porder)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_status = FileStatus::Updated;

	std::map<OrderKey, OneOrderPtrs>::iterator pos = m_orders.find(porder->orderKey);

	if (pos != m_orders.end())
	{
		pos->second.insert(porder);
	}
	else
	{
		OneOrderPtrs values;
		values.insert(porder);
		m_orders.insert(std::pair<OrderKey, OneOrderPtrs>(porder->orderKey, values));
	}

}

OneOrderPtr Cdb_Order_Real::RemoveOne(const OrderKey& key, int localOrderNo)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	OneOrderPtr back = nullptr;
	m_status = FileStatus::Updated;

	OneOrderPtr targetOrder = std::make_shared<COneOrder>();
	targetOrder->orderKey = key;
	targetOrder->localOrderNo = localOrderNo;

	std::map<OrderKey, OneOrderPtrs>::iterator pos = m_orders.find(key);
	if (pos == m_orders.end()) return nullptr;

	OneOrderPtrs::iterator pset = pos->second.find(targetOrder);
	if (pset == pos->second.end()) return nullptr;

	back = *pset;
	pos->second.erase(pset);

	return back;

}

OneOrderPtr Cdb_Order_Real::GetOne(const OrderKey& key, int localOrderNo)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	OneOrderPtr targetOrder = std::make_shared<COneOrder>();
	targetOrder->orderKey = key;
	targetOrder->localOrderNo = localOrderNo;

	std::map<OrderKey, OneOrderPtrs>::iterator pos = m_orders.find(key);
	if (pos == m_orders.end()) return nullptr;

	OneOrderPtrs::iterator pset = pos->second.find(targetOrder);
	if (pset == pos->second.end()) return nullptr;

	return *pset;
}

OneOrderPtr Cdb_Order_Real::GetOne(int localOrderNo)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (const auto& mapitem : m_orders)
	{
		for (const auto& pset : mapitem.second)
		{
			if (pset->localOrderNo != localOrderNo) continue;
			return pset;
		}
	}
	return nullptr;
}

OneOrderPtrs Cdb_Order_Real::GetAll(const OrderKey& key)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	OneOrderPtrs back;
	std::map<OrderKey, OneOrderPtrs>::iterator pos = m_orders.find(key);
	if (pos == m_orders.end()) return back;

	return pos->second;

}
