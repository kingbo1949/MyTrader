#pragma once
#include "db_ActiveAction.h"
class Cdb_ActiveAction_Real : public Cdb_ActiveAction
{
public:
	Cdb_ActiveAction_Real():m_status(FileStatus::Saved){ ; };
	virtual ~Cdb_ActiveAction_Real() { ; };

	virtual FileStatus	GetStatus() override final;

	virtual void		SetStatus(FileStatus fileStatus) override final;

	virtual void		AddOne(ActionPtr pAction) override final;

	virtual ActionPtr	GetOne(const ActionKey& key, int localOrderNo) override final;

	virtual void		RemoveOne(const ActionKey& key, int localOrderNo) override final;

	virtual ActionPtrs	GetAll(const ActionKey& key) override final;

protected:
	std::mutex						m_mutex;
	std::map<ActionKey, ActionPtrs>	m_actions;
	FileStatus						m_status;

};

