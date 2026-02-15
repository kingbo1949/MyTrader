#include "CDbTable_Atr.h"
#include "../Factory.h"
#include <RocksWriteBatch.h>
#include <climits>

CDbTable_Atr::CDbTable_Atr(CRocksEnv& env, const std::string& prefix)
	: m_env(env), m_prefix(prefix)
{
	for (auto tt : ALL_TIME_TYPES)
	{
		GetTable(tt);
	}
	printf("CDbTable_Atr open, %zu column families ready\n", m_tables.size());
}

CRocksTable<IAtrValue>& CDbTable_Atr::GetTable(ITimeType timeType)
{
	auto it = m_tables.find(timeType);
	if (it != m_tables.end()) {
		return *it->second;
	}
	std::string cfName = m_prefix + "_" + Trans_Str(timeType);
	auto table = std::make_unique<CRocksTable<IAtrValue>>(m_env, cfName);
	auto& ref = *table;
	m_tables[timeType] = std::move(table);
	return ref;
}

void CDbTable_Atr::AddOne(const std::string& codeId, ITimeType timeType, const IAtrValue& value)
{
	CRocksKey key(codeId, value.time);
	GetTable(timeType).Put(key, value);
}

void CDbTable_Atr::AddSome(const std::string& codeId, ITimeType timeType, const IAtrValues& values)
{
	CRocksWriteBatch batch(m_env);
	auto* cfHandle = GetTable(timeType).GetCFHandle();
	for (const auto& v : values)
	{
		CRocksKey key(codeId, v.time);
		std::string data = RocksSerializer<IAtrValue>::Serialize(v);
		batch.Put(cfHandle, key, data);
	}
	batch.Commit();
}

bool CDbTable_Atr::GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IAtrValue& value)
{
	CRocksKey key(codeId, timePos);
	return GetTable(timeType).Get(key, &value);
}

void CDbTable_Atr::RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos)
{
	CRocksKey key(codeId, timePos);
	GetTable(timeType).Delete(key);
}

void CDbTable_Atr::RemoveKey(const std::string& codeId, ITimeType timeType)
{
	GetTable(timeType).DeleteRange(codeId, 0, LLONG_MAX);
}

void CDbTable_Atr::RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime)
{
	GetTable(timeType).DeleteRange(codeId, beginTime, endTime);
}

void CDbTable_Atr::RemoveAll()
{
	for (auto& [timeType, table] : m_tables) {
		CRocksKey beginKey("", 0);
		CRocksKey endKey("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
		                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
		                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
		                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff",
		                 LLONG_MAX);
		rocksdb::WriteOptions wo;
		m_env.GetDB()->DeleteRange(wo, table->GetCFHandle(), beginKey.ToSlice(), endKey.ToSlice());
	}
}

void CDbTable_Atr::GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IAtrValues& values)
{
	if (query.byReqType == 0) {
		GetBackWard(codeId, timeType, LLONG_MAX, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 1) {
		GetRange(codeId, timeType, 0, LLONG_MAX, values);
		return;
	}
	if (query.byReqType == 2) {
		GetBackWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 3) {
		GetForWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 4) {
		GetRange(codeId, timeType, query.timePair.beginPos, query.timePair.endPos, values);
		return;
	}
}

void CDbTable_Atr::GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IAtrValues& values)
{
	values.clear();
	GetTable(timeType).ScanByRange(codeId, beginTime, endTime,
		[&values](const CRocksKey& k, const IAtrValue& v) {
			values.push_back(v);
			return true;
		});
}

void CDbTable_Atr::GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IAtrValues& values)
{
	values.clear();
	Long remaining = count;
	GetTable(timeType).ScanByRange(codeId, beginTime, LLONG_MAX,
		[&values, &remaining](const CRocksKey& k, const IAtrValue& v) {
			values.push_back(v);
			return (--remaining > 0);
		});
}

void CDbTable_Atr::GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IAtrValues& values)
{
	values.clear();
	std::vector<std::pair<CRocksKey, IAtrValue>> results;
	GetTable(timeType).ScanBackwardN(codeId, endTime, static_cast<size_t>(count), results);
	values.reserve(results.size());
	for (auto& [k, v] : results) {
		values.push_back(std::move(v));
	}
}
