#include "DbTable_DivType.h"
#include "../Factory.h"
#include <RocksWriteBatch.h>
#include <climits>

CDbTable_DivType::CDbTable_DivType(CRocksEnv& env, const std::string& prefix)
	: m_env(env), m_prefix(prefix)
{
	for (auto tt : ALL_TIME_TYPES)
	{
		GetTable(tt);
	}
	printf("CDbTable_DivType open, %zu column families ready\n", m_tables.size());
}

CRocksTable<IDivTypeValue>& CDbTable_DivType::GetTable(ITimeType timeType)
{
	auto it = m_tables.find(timeType);
	if (it != m_tables.end()) {
		return *it->second;
	}
	std::string cfName = m_prefix + "_" + Trans_Str(timeType);
	auto table = std::make_unique<CRocksTable<IDivTypeValue>>(m_env, cfName);
	auto& ref = *table;
	m_tables[timeType] = std::move(table);
	return ref;
}

void CDbTable_DivType::AddOne(const std::string& codeId, ITimeType timeType, const IDivTypeValue& value)
{
	CRocksKey key(codeId, value.time);
	GetTable(timeType).Put(key, value);
}

void CDbTable_DivType::AddSome(const std::string& codeId, ITimeType timeType, const IDivTypeValues& values)
{
	CRocksWriteBatch batch(m_env);
	auto* cfHandle = GetTable(timeType).GetCFHandle();
	for (const auto& v : values)
	{
		CRocksKey key(codeId, v.time);
		std::string data = RocksSerializer<IDivTypeValue>::Serialize(v);
		batch.Put(cfHandle, key, data);
	}
	batch.Commit();
}

bool CDbTable_DivType::GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IDivTypeValue& value)
{
	CRocksKey key(codeId, timePos);
	return GetTable(timeType).Get(key, &value);
}

void CDbTable_DivType::RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos)
{
	CRocksKey key(codeId, timePos);
	GetTable(timeType).Delete(key);
}

void CDbTable_DivType::RemoveKey(const std::string& codeId, ITimeType timeType)
{
	GetTable(timeType).DeleteRange(codeId, 0, LLONG_MAX);
}

void CDbTable_DivType::RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime)
{
	GetTable(timeType).DeleteRange(codeId, beginTime, endTime);
}

void CDbTable_DivType::RemoveAll()
{
	for (auto& [timeType, table] : m_tables) {
		CRocksKey beginKey("", 0);
		CRocksKey endKey("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
		                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
		                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
		                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff",
		                 LLONG_MAX);
		const auto& wo = m_env.GetWriteOptions();
		m_env.GetDB()->DeleteRange(wo, table->GetCFHandle(), beginKey.ToSlice(), endKey.ToSlice());
	}
}

void CDbTable_DivType::GetValues(const std::string& codeId, ITimeType timeType, IQuery query, IDivTypeValues& values)
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
		// 表示请求某个时间以前(包括该时间)多少个单位的数据(dwSubscribeNum为0时表示该时间前所有的数据)
		if (query.dwSubscribeNum == 0) query.dwSubscribeNum = LLONG_MAX;
		GetBackWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 3) {
		// 表示请求某个时间以后多少个单位的数据(dwSubscribeNum为0时表示该时间后所有的数据)
		if (query.dwSubscribeNum == 0) query.dwSubscribeNum = LLONG_MAX;
		GetForWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 4) {
		GetRange(codeId, timeType, query.timePair.beginPos, query.timePair.endPos, values);
		return;
	}
}

void CDbTable_DivType::GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IDivTypeValues& values)
{
	values.clear();
	GetTable(timeType).ScanByRange(codeId, beginTime, endTime,
		[&values](const CRocksKey& k, const IDivTypeValue& v) {
			values.push_back(v);
			return true;
		});
}

void CDbTable_DivType::GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IDivTypeValues& values)
{
	values.clear();
	Long remaining = count;
	GetTable(timeType).ScanByRange(codeId, beginTime, LLONG_MAX,
		[&values, &remaining](const CRocksKey& k, const IDivTypeValue& v) {
			values.push_back(v);
			return (--remaining > 0);
		});
}

void CDbTable_DivType::GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IDivTypeValues& values)
{
	values.clear();
	std::vector<std::pair<CRocksKey, IDivTypeValue>> results;
	GetTable(timeType).ScanBackwardN(codeId, endTime, static_cast<size_t>(count), results);
	values.reserve(results.size());
	for (auto& [k, v] : results) {
		values.push_back(std::move(v));
	}
}
