#include "DbTable_TickHis.h"
#include <RocksWriteBatch.h>
#include <climits>
#include <chrono>

CDbTable_TickHis::CDbTable_TickHis(CRocksEnv& env, const std::string& cfName)
	: m_env(env)
{
	m_table = std::make_unique<CRocksTable<ITick>>(env, cfName);
	printf("CDbTable_TickHis open\n");
}

void CDbTable_TickHis::AddOne(const ITick& value)
{
	CRocksKey key(value.codeId, value.time);
	m_table->Put(key, value);
}

void CDbTable_TickHis::AddSome(const ITicks& values)
{
	CRocksWriteBatch batch(m_env);
	auto* cfHandle = m_table->GetCFHandle();
	for (const auto& v : values)
	{
		CRocksKey key(v.codeId, v.time);
		std::string data = RocksSerializer<ITick>::Serialize(v);
		batch.Put(cfHandle, key, data);
	}
	batch.Commit();
}

bool CDbTable_TickHis::GetOne(const std::string& codeId, Long timePos, ITick& value)
{
	CRocksKey key(codeId, timePos);
	return m_table->Get(key, &value);
}

void CDbTable_TickHis::RemoveOne(const std::string& codeId, Long timePos)
{
	CRocksKey key(codeId, timePos);
	m_table->Delete(key);
}

void CDbTable_TickHis::RemoveKey(const std::string& codeId)
{
	m_table->DeleteRange(codeId, 0, LLONG_MAX);
}

void CDbTable_TickHis::RemoveByRange(const std::string& codeId, Long beginTime, Long endTime)
{
	m_table->DeleteRange(codeId, beginTime, endTime);
}

void CDbTable_TickHis::RemoveAll()
{
	CRocksKey beginKey("", 0);
	CRocksKey endKey("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
	                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
	                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
	                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff",
	                 LLONG_MAX);
	const auto& wo = m_env.GetWriteOptions();
	m_env.GetDB()->DeleteRange(wo, m_table->GetCFHandle(), beginKey.ToSlice(), endKey.ToSlice());
}

void CDbTable_TickHis::GetTicks(const std::string& codeId, IQuery query, ITicks& values)
{
	if (query.byReqType == 0) {
		GetBackWard(codeId, LLONG_MAX, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 1) {
		GetRange(codeId, 0, LLONG_MAX, values);
		return;
	}
	if (query.byReqType == 2) {
		// 表示请求某个时间以前(包括该时间)多少个单位的数据(dwSubscribeNum为0时表示该时间前所有的数据)
		if (query.dwSubscribeNum == 0) query.dwSubscribeNum = LLONG_MAX;
		GetBackWard(codeId, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 3) {
		// 表示请求某个时间以后多少个单位的数据(dwSubscribeNum为0时表示该时间后所有的数据)
		if (query.dwSubscribeNum == 0) query.dwSubscribeNum = LLONG_MAX;
		GetForWard(codeId, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 4) {
		GetRange(codeId, query.timePair.beginPos, query.timePair.endPos, values);
		return;
	}
}

bool CDbTable_TickHis::GetLastTick(const std::string& codeId, ITick& value)
{
	IQuery query;
	query.byReqType = 2;
	query.dwSubscribeNum = 1;
	query.tTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count() + 1;

	ITicks values;
	GetTicks(codeId, query, values);
	if (values.empty()) return false;

	value = values.back();
	return true;
}

void CDbTable_TickHis::GetRange(const std::string& codeId, Long beginTime, Long endTime, ITicks& values)
{
	values.clear();
	m_table->ScanByRange(codeId, beginTime, endTime,
		[&values](const CRocksKey& k, const ITick& v) {
			values.push_back(v);
			return true;
		});
}

void CDbTable_TickHis::GetForWard(const std::string& codeId, Long beginTime, Long count, ITicks& values)
{
	values.clear();
	Long remaining = count;
	m_table->ScanByRange(codeId, beginTime, LLONG_MAX,
		[&values, &remaining](const CRocksKey& k, const ITick& v) {
			values.push_back(v);
			return (--remaining > 0);
		});
}

void CDbTable_TickHis::GetBackWard(const std::string& codeId, Long endTime, Long count, ITicks& values)
{
	values.clear();
	std::vector<std::pair<CRocksKey, ITick>> results;
	m_table->ScanBackwardN(codeId, endTime, static_cast<size_t>(count), results);
	values.reserve(results.size());
	for (auto& [k, v] : results) {
		values.push_back(std::move(v));
	}
}
