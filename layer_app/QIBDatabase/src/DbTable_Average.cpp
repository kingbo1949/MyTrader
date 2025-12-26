#include "DbTable_Average.h"

#include "Compare.h"
Bdb::CDbTable_Average::CDbTable_Average(DbEnv* env, const std::string& path, const std::string& dbName)
	:CDbTable(env, path, dbName)
{
	ParamForSetupDB param;
	param.env = m_env;
	param.path = m_path;
	param.dbName = m_dbName;
	param.func_key = CCompare::QKeyA;
	param.func_value = CCompare::AvgValueA;
	param.partNum = 0;
	param.func_partition = NULL;

	m_pDb = new CDb<IQKey, IAvgValue>(param);
	m_pDb->Open();
	printf("CDbTable_MA open \n");
}

void Bdb::CDbTable_Average::AddOne(const std::string& codeId, ITimeType timeType, const IAvgValue& value)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;
	m_pDb->AddOne(key, value);

}

void Bdb::CDbTable_Average::AddSome(const std::string& codeId, ITimeType timeType, const IAvgValues& values)
{
	for (const auto& onevalue : values)
	{
		AddOne(codeId, timeType, onevalue);
	}
}

bool Bdb::CDbTable_Average::GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IAvgValue& value)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	value.time = timePos;
	return m_pDb->GetOne(key, value);

}

void Bdb::CDbTable_Average::RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IAvgValue value;
	value.time = timePos;

	m_pDb->RemoveOne(std::make_pair(key, value));

}

void Bdb::CDbTable_Average::RemoveKey(const std::string& codeId, ITimeType timeType)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	m_pDb->RemoveOneKey(key);

}

void Bdb::CDbTable_Average::RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IAvgValue begin;
	begin.time = beginTime;

	IAvgValue end;
	end.time = endTime;

	CDb<IQKey, IAvgValue>::FieldPairRange fieldPairRange;
	fieldPairRange.beginPair = std::make_pair(key, begin);
	fieldPairRange.endPair = std::make_pair(key, end);

	m_pDb->RemoveRange(fieldPairRange);
	return;


}

void Bdb::CDbTable_Average::RemoveAll()
{
	m_pDb->RemoveAll();
}

void Bdb::CDbTable_Average::GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IAvgValues& values)
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

void Bdb::CDbTable_Average::GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IAvgValues& values)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IAvgValue begin;
	begin.time = beginTime;

	IAvgValue end;
	end.time = endTime;

	CDb<IQKey, IAvgValue>::FieldPairRange fieldPairRange;
	fieldPairRange.beginPair = std::make_pair(key, begin);
	fieldPairRange.endPair = std::make_pair(key, end);

	m_pDb->GetRange(fieldPairRange, values);

	return;

}

void Bdb::CDbTable_Average::GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IAvgValues& values)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IAvgValue kline;
	kline.time = beginTime;
	CDb<IQKey, IAvgValue>::FieldPair pair = std::make_pair(key, kline);
	m_pDb->GetForWardInDup(pair, count, values);


}

void Bdb::CDbTable_Average::GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IAvgValues& values)
{
	IQKey key;
	key.codeId = codeId;
	key.timeType = timeType;

	IAvgValue kline;
	kline.time = endTime;
	CDb<IQKey, IAvgValue>::FieldPair pair = std::make_pair(key, kline);

	m_pDb->GetBackWardInDup(pair, count, values);

}
