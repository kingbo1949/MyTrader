#include "DbTable_KLine.h"
#include "Compare.h"
#include <Global.h>

Bdb::CDbTable_KLine::CDbTable_KLine(DbEnv* env, const std::string& path, const std::string& dbName)
	:CDbTable(env, path, dbName)
{
	ParamForSetupDB param;
	param.env = m_env;
	param.path = m_path;
	param.dbName = m_dbName;
	param.func_key = CCompare::QKeyA;
	param.func_value = CCompare::KLineA;
	param.partNum = 0;
	param.func_partition = NULL;

	m_pDb = new CDb<IQKey, IKLine>(param);
	m_pDb->Open();
	printf("CDbTable_KLine open \n");


}

void Bdb::CDbTable_KLine::AddOne(const std::string& codeId, ITimeType timeType, const IKLine& value)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;
	m_pDb->AddOne(key, value);

}

void Bdb::CDbTable_KLine::AddSome(const std::string& codeId, ITimeType timeType, const IKLines& values)
{
	for (IKLines::const_iterator pos = values.begin(); pos != values.end(); ++pos)
	{
		AddOne(codeId, timeType, *pos);
	}

}

bool Bdb::CDbTable_KLine::GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IKLine& value)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	value.time = timePos;
	value.close = 0;
	value.open = 0;
	value.high = 0;
	value.low = 0;
	value.vol = 0;

	return m_pDb->GetOne(key, value);

}

void Bdb::CDbTable_KLine::RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IKLine value;
	value.time = timePos;

	m_pDb->RemoveOne(std::make_pair(key, value));

}

void Bdb::CDbTable_KLine::RemoveKey(const std::string& codeId, ITimeType timeType)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	m_pDb->RemoveOneKey(key);

}

void Bdb::CDbTable_KLine::RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IKLine begin;
	begin.time = beginTime;

	IKLine end;
	end.time = endTime;

	CDb<IQKey, IKLine>::FieldPairRange fieldPairRange;
	fieldPairRange.beginPair = std::make_pair(key, begin);
	fieldPairRange.endPair = std::make_pair(key, end);

	m_pDb->RemoveRange(fieldPairRange);
	return;


}

void Bdb::CDbTable_KLine::RemoveAll()
{
	m_pDb->RemoveAll();
}

void Bdb::CDbTable_KLine::GetKLines(const std::string& codeId, ITimeType timeType, const IQuery& query, IKLines& values)
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
		GetBackWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;

	}
	if (query.byReqType == 3)
	{
		GetForWard(codeId, timeType, query.tTime, query.dwSubscribeNum, values);
		return;

	}
	if (query.byReqType == 4)
	{
		GetRange(codeId, timeType, query.timePair.beginPos, query.timePair.endPos, values);
		return;

	}
	return;

}

void Bdb::CDbTable_KLine::GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IKLines& values)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IKLine begin;
	begin.time = beginTime;

	IKLine end;
	end.time = endTime;

	CDb<IQKey, IKLine>::FieldPairRange fieldPairRange;
	fieldPairRange.beginPair = std::make_pair(key, begin);
	fieldPairRange.endPair = std::make_pair(key, end);

	m_pDb->GetRange(fieldPairRange, values);

	return;

}

void Bdb::CDbTable_KLine::GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IKLines& values)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IKLine kline;
	kline.time = beginTime;
	CDb<IQKey, IKLine>::FieldPair pair = std::make_pair(key, kline);
	m_pDb->GetForWardInDup(pair, count, values);

}

void Bdb::CDbTable_KLine::GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IKLines& values)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IKLine kline;
	kline.time = endTime;
	CDb<IQKey, IKLine>::FieldPair pair = std::make_pair(key, kline);

	m_pDb->GetBackWardInDup(pair, count, values);

}
