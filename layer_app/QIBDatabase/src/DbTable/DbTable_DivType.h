#pragma once
#include <RocksTable.h>
#include <QStruc.h>
#include "../RocksTypes.h"
#include <map>
#include <memory>
using namespace IBTrader;
using namespace Ice;
class CDbTable_DivType
{
public:
	CDbTable_DivType(CRocksEnv& env, const std::string& prefix);
	~CDbTable_DivType() = default;

	void		AddOne(const std::string& codeId, ITimeType timeType, const IDivTypeValue& value);
	void		AddSome(const std::string& codeId, ITimeType timeType, const IDivTypeValues& values);
	bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IDivTypeValue& value);

	void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
	void		RemoveKey(const std::string& codeId, ITimeType timeType);
	void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
	void		RemoveAll();

	void		GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IDivTypeValues& values);

private:
	CRocksEnv& m_env;
	std::string m_prefix;
	std::map<ITimeType, std::unique_ptr<CRocksTable<IDivTypeValue>>> m_tables;

	CRocksTable<IDivTypeValue>& GetTable(ITimeType timeType);

	void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IDivTypeValues& values);
	void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IDivTypeValues& values);
	void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IDivTypeValues& values);
};
