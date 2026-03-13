#include "DbTable_KLine.h"
#include "../Factory.h"
#include <RocksWriteBatch.h>
#include <climits>
#include <Factory_Log.h>
#include <Global.h>
CDbTable_KLine::CDbTable_KLine(CRocksEnv& env, const std::string& prefix)
	: m_env(env), m_prefix(prefix)
{
	for (auto tt : ALL_TIME_TYPES)
	{
		GetTable(tt);
	}
	Log_Print(LogLevel::Info, fmt::format("CDbTable_KLine open, {} column families ready", m_tables.size()));
}

CRocksTable<IKLine>& CDbTable_KLine::GetTable(ITimeType timeType)
{
	auto it = m_tables.find(timeType);
	if (it != m_tables.end()) {
		return *it->second;
	}
	// 列族名 = 前缀_周期，如 "KLINE_M5"
	std::string cfName = m_prefix + "_" + Trans_Str(timeType);
	auto table = std::make_unique<CRocksTable<IKLine>>(m_env, cfName);
	auto& ref = *table;
	m_tables[timeType] = std::move(table);
	return ref;
}

void CDbTable_KLine::AddOne(const std::string& codeId, ITimeType timeType, const IKLine& value)
{
	CRocksKey key(codeId, value.time);
	GetTable(timeType).Put(key, value);
}

void CDbTable_KLine::AddSome(const std::string& codeId, ITimeType timeType, const IKLines& values)
{
	CRocksWriteBatch batch(m_env);
	auto* cfHandle = GetTable(timeType).GetCFHandle();
	for (const auto& v : values)
	{
		CRocksKey key(codeId, v.time);
		std::string data = RocksSerializer<IKLine>::Serialize(v);
		batch.Put(cfHandle, key, data);
	}
	batch.Commit();
}

bool CDbTable_KLine::GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IKLine& value)
{
	CRocksKey key(codeId, timePos);
	return GetTable(timeType).Get(key, &value);
}

void CDbTable_KLine::RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos)
{
	CRocksKey key(codeId, timePos);
	GetTable(timeType).Delete(key);
}

void CDbTable_KLine::RemoveKey(const std::string& codeId, ITimeType timeType)
{
	// 删除该合约在此周期的所有数据
	GetTable(timeType).DeleteRange(codeId, 0, LLONG_MAX);
}

void CDbTable_KLine::RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime)
{
	GetTable(timeType).DeleteRange(codeId, beginTime, endTime);
}

void CDbTable_KLine::RemoveAll()
{
	// 清空所有已打开的周期表
	for (auto& [timeType, table] : m_tables) {
		// RocksDB 没有 RemoveAll，用最大范围 DeleteRange 模拟
		// 这会删除所有合约的数据
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

void CDbTable_KLine::GetKLines(const std::string& codeId, ITimeType timeType, IQuery query, IKLines& values)
{
	if (query.byReqType == 0)
	{
		GetBackWard(codeId, timeType, LLONG_MAX, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 1)
	{
		GetRange(codeId, timeType, 0, LLONG_MAX, values);
		return;
	}
	if (query.byReqType == 2)
	{
		// 表示请求某个时间以前(包括该时间)多少个单位的数据(dwSubscribeNum为0时表示该时间前所有的数据)
		if (query.dwSubscribeNum == 0) query.dwSubscribeNum = LLONG_MAX;
		GetBackWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 3)
	{
		// 表示请求某个时间以后多少个单位的数据(dwSubscribeNum为0时表示该时间后所有的数据)
		if (query.dwSubscribeNum == 0) query.dwSubscribeNum = LLONG_MAX;
		GetForWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 4)
	{
		GetRange(codeId, timeType, query.timePair.beginPos, query.timePair.endPos, values);
		return;
	}
}

bool CDbTable_KLine::GetLastDayKLine(std::string codeId, Long msTime, IKLine &kline)
{
	// 得到机器时间day
	time_t seconds = msTime / 1000;
	time_t msDay = CGlobal::QGetDayTime(seconds) * 1000;

	time_t belongDay = 0;
	if (msTime < msDay + 17 * 60 * 60 * 1000)
	{
		// msTime是一个小于17:00:00的时间
		belongDay = msDay;
	}else
	{
		// msTime是一个17:00:00 - 24:00:00 的时间
		belongDay = msDay + 24 * 60 * 60 * 1000;
	}
	IQuery query;
	query.byReqType = 2;		// 2表示请求某个时间以前(包括该时间)多少个单位的数据(dwSubscribeNum为0时表示该时间前所有的数据)
	query.tTime = belongDay - 1000; // 时间(请求类型为2、3时使用)(毫秒) 数据区间为[... : tTime] 或者 [tTime : ...]
	query.dwSubscribeNum = 1;

	IKLines klines;
	GetKLines(codeId, ITimeType::D1, query, klines);

	if (klines.size() != 1) return false;
	kline = klines[0];
	return true;

}

void CDbTable_KLine::GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IKLines& values)
{
	values.clear();
	GetTable(timeType).ScanByRange(codeId, beginTime, endTime,
		[&values](const CRocksKey& k, const IKLine& v) {
			values.push_back(v);
			return true;
		});
}

void CDbTable_KLine::GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IKLines& values)
{
	values.clear();
	Long remaining = count;
	GetTable(timeType).ScanByRange(codeId, beginTime, LLONG_MAX,
		[&values, &remaining](const CRocksKey& k, const IKLine& v) {
			values.push_back(v);
			return (--remaining > 0);
		});
}

void CDbTable_KLine::GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IKLines& values)
{
	values.clear();
	std::vector<std::pair<CRocksKey, IKLine>> results;
	GetTable(timeType).ScanBackwardN(codeId, endTime, static_cast<size_t>(count), results);
	values.reserve(results.size());
	for (auto& [k, v] : results) {
		values.push_back(std::move(v));
	}
}
