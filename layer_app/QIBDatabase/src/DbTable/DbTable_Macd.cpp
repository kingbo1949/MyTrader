#include "DbTable_Macd.h"
#include "../Factory.h"
#include <RocksWriteBatch.h>
#include <climits>

CDbTable_Macd::CDbTable_Macd(CRocksEnv& env, const std::string& prefix)
	: m_env(env), m_prefix(prefix)
{
	for (auto tt : ALL_TIME_TYPES)
	{
		GetTable(tt);
	}
	printf("CDbTable_Macd open, %zu column families ready\n", m_tables.size());
}

CRocksTable<IMacdValue>& CDbTable_Macd::GetTable(ITimeType timeType)
{
	auto it = m_tables.find(timeType);
	if (it != m_tables.end()) {
		return *it->second;
	}
	std::string cfName = m_prefix + "_" + Trans_Str(timeType);
	auto table = std::make_unique<CRocksTable<IMacdValue>>(m_env, cfName);
	auto& ref = *table;
	m_tables[timeType] = std::move(table);
	return ref;
}

void CDbTable_Macd::AddOne(const std::string& codeId, ITimeType timeType, const IMacdValue& value)
{
	CRocksKey key(codeId, value.time);
	GetTable(timeType).Put(key, value);
}

void CDbTable_Macd::AddSome(const std::string& codeId, ITimeType timeType, const IMacdValues& values)
{
	CRocksWriteBatch batch(m_env);
	auto* cfHandle = GetTable(timeType).GetCFHandle();
	for (const auto& v : values)
	{
		CRocksKey key(codeId, v.time);
		std::string data = RocksSerializer<IMacdValue>::Serialize(v);
		batch.Put(cfHandle, key, data);
	}
	batch.Commit();
}

bool CDbTable_Macd::GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IMacdValue& value)
{
	CRocksKey key(codeId, timePos);
	return GetTable(timeType).Get(key, &value);
}

void CDbTable_Macd::RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos)
{
	CRocksKey key(codeId, timePos);
	GetTable(timeType).Delete(key);
}

void CDbTable_Macd::RemoveKey(const std::string& codeId, ITimeType timeType)
{
	GetTable(timeType).DeleteRange(codeId, 0, LLONG_MAX);
}

void CDbTable_Macd::RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime)
{
	GetTable(timeType).DeleteRange(codeId, beginTime, endTime);
}

void CDbTable_Macd::RemoveAll()
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

void CDbTable_Macd::GetValues(const std::string& codeId, ITimeType timeType, IQuery query, IMacdValues& values)
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

void CDbTable_Macd::GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IMacdValues& values)
{
	values.clear();
	GetTable(timeType).ScanByRange(codeId, beginTime, endTime,
		[&values](const CRocksKey& k, const IMacdValue& v) {
			values.push_back(v);
			return true;
		});
}

void CDbTable_Macd::GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IMacdValues& values)
{
	values.clear();
	Long remaining = count;
	GetTable(timeType).ScanByRange(codeId, beginTime, LLONG_MAX,
		[&values, &remaining](const CRocksKey& k, const IMacdValue& v) {
			values.push_back(v);
			return (--remaining > 0);
		});
}

void CDbTable_Macd::GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IMacdValues& values)
{
	values.clear();
	std::vector<std::pair<CRocksKey, IMacdValue>> results;
	GetTable(timeType).ScanBackwardN(codeId, endTime, static_cast<size_t>(count), results);
	values.reserve(results.size());
	for (auto& [k, v] : results) {
		values.push_back(std::move(v));
	}
}
