#pragma once
#include <RocksTable.h>
#include <QStruc.h>
#include "../RocksTypes.h"
#include <map>
#include <memory>
using namespace IBTrader;
using namespace Ice;
class CDbTable_Atr
{
public:
	CDbTable_Atr(CRocksEnv& env, const std::string& prefix);
	~CDbTable_Atr() = default;

	void		AddOne(const std::string& codeId, ITimeType timeType, const IAtrValue& value);
	void		AddSome(const std::string& codeId, ITimeType timeType, const IAtrValues& values);
	bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IAtrValue& value);

	void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
	void		RemoveKey(const std::string& codeId, ITimeType timeType);
	void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
	void		RemoveAll();

	void		GetValues(const std::string& codeId, ITimeType timeType, IQuery query, IAtrValues& values);

private:
	CRocksEnv& m_env;
	std::string m_prefix;
	std::map<ITimeType, std::unique_ptr<CRocksTable<IAtrValue>>> m_tables;

	CRocksTable<IAtrValue>& GetTable(ITimeType timeType);

	void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IAtrValues& values);
	void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IAtrValues& values);
	void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IAtrValues& values);
};
