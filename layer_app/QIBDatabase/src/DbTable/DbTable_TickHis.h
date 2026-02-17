#pragma once
#include <RocksTable.h>
#include <QStruc.h>
#include "../RocksTypes.h"
#include <memory>
using namespace IBTrader;
using namespace Ice;
class CDbTable_TickHis
{
public:
	CDbTable_TickHis(CRocksEnv& env, const std::string& cfName);
	~CDbTable_TickHis() = default;

	void		AddOne(const ITick& value);
	void		AddSome(const ITicks& values);
	bool		GetOne(const std::string& codeId, Long timePos, ITick& value);
	void		RemoveOne(const std::string& codeId, Long timePos);
	void		RemoveKey(const std::string& codeId);

	void		RemoveByRange(const std::string& codeId, Long beginTime, Long endTime);
	void		GetTicks(const std::string& codeId, IQuery query, ITicks& values);
	void		RemoveAll();
	bool		GetLastTick(const std::string& codeId, ITick& value);

private:
	CRocksEnv& m_env;
	std::unique_ptr<CRocksTable<ITick>> m_table;

	void		GetRange(const std::string& codeId, Long beginTime, Long endTime, ITicks& values);
	void		GetForWard(const std::string& codeId, Long beginTime, Long count, ITicks& values);
	void		GetBackWard(const std::string& codeId, Long endTime, Long count, ITicks& values);
};
