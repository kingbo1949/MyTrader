#pragma once
#include <RocksTable.h>
#include <QStruc.h>
#include "../RocksTypes.h"
#include <map>
#include <memory>
using namespace IBTrader;
using namespace Ice;
class CDbTable_Average
{
public:
	CDbTable_Average(CRocksEnv& env, const std::string& prefix);
	~CDbTable_Average() = default;

	void		AddOne(const std::string& codeId, ITimeType timeType, const IAvgValue& value);
	void		AddSome(const std::string& codeId, ITimeType timeType, const IAvgValues& values);
	bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IAvgValue& value);

	void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
	void		RemoveKey(const std::string& codeId, ITimeType timeType);
	void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
	void		RemoveAll();

	void		GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IAvgValues& values);

private:
	CRocksEnv& m_env;
	std::string m_prefix;
	std::map<ITimeType, std::unique_ptr<CRocksTable<IAvgValue>>> m_tables;

	CRocksTable<IAvgValue>& GetTable(ITimeType timeType);

	void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IAvgValues& values);
	void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IAvgValues& values);
	void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IAvgValues& values);
};
