#include "pch.h"
#include "db_ActiveAction_Real.h"

FileStatus Cdb_ActiveAction_Real::GetStatus()
{
	return m_status;
}

void Cdb_ActiveAction_Real::SetStatus(FileStatus fileStatus)
{
	m_status = fileStatus;
}

void Cdb_ActiveAction_Real::AddOne(ActionPtr pAction)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_status = FileStatus::Updated;
	std::map<ActionKey, ActionPtrs>::iterator pos = m_actions.find(pAction->key);
	if (pos != m_actions.end())
	{
		pos->second.insert(pAction);
	}
	else
	{
		ActionPtrs values;
		values.insert(pAction);
		m_actions.insert(std::pair<ActionKey, ActionPtrs>(pAction->key, values));
	}
}

ActionPtr Cdb_ActiveAction_Real::GetOne(const ActionKey& key, int localOrderNo)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	ActionPtr targetAction = std::make_shared<CAction>();
	targetAction->key = key;
	targetAction->localOrderNo = localOrderNo;

	std::map<ActionKey, ActionPtrs>::iterator pos = m_actions.find(key);
	if (pos == m_actions.end()) return nullptr;

	ActionPtrs::iterator pset = pos->second.find(targetAction);
	if (pset == pos->second.end()) return nullptr;
	
	return *pset;
}

void Cdb_ActiveAction_Real::RemoveOne(const ActionKey& key, int localOrderNo)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_status = FileStatus::Updated;

	ActionPtr targetAction = std::make_shared<CAction>();
	targetAction->key = key;
	targetAction->localOrderNo = localOrderNo;

	std::map<ActionKey, ActionPtrs>::iterator pos = m_actions.find(key);
	if (pos == m_actions.end()) return;

	ActionPtrs::iterator pset = pos->second.find(targetAction);
	if (pset == pos->second.end()) return;

	pos->second.erase(pset);
	return;
}

ActionPtrs Cdb_ActiveAction_Real::GetAll(const ActionKey& key)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	ActionPtrs back;
	std::map<ActionKey, ActionPtrs>::iterator pos = m_actions.find(key);
	if (pos == m_actions.end()) return back;

	return pos->second;
}
