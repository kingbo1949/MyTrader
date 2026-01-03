#include "DbTable_TickHis.h"
#include "../Compare.h"


Bdb::CDbTable_TickHis::CDbTable_TickHis(DbEnv* env, const std::string& path, const std::string& dbName)
	:CDbTable(env, path, dbName)
{
	ParamForSetupDB param;
	param.env = m_env;
	param.path = m_path;
	param.dbName = m_dbName;
	param.func_key = CCompare::CodeIdA;
	param.func_value = CCompare::TickA;
	param.partNum = 0;
	param.func_partition = NULL;

	m_pDb = new CDb<std::string, ITick>(param);
	m_pDb->Open();
	printf("CDbTable_TickHis open \n");

}

Bdb::CDbTable_TickHis::~CDbTable_TickHis(void)
{

}

void Bdb::CDbTable_TickHis::AddOne(const ITick& value)
{
	m_pDb->AddOne(value.codeId, value);
}

void Bdb::CDbTable_TickHis::AddSome(const ITicks& values)
{
	for (ITicks::const_iterator pos = values.begin(); pos != values.end(); ++pos)
	{
		AddOne(*pos);
	}
}

bool Bdb::CDbTable_TickHis::GetOne(const std::string& codeId, Long timePos, ITick& value)
{
	value.time = timePos;
	return m_pDb->GetOne(codeId, value);

}

void Bdb::CDbTable_TickHis::RemoveOne(const std::string& codeId, Long timePos)
{
	ITick value;
	value.codeId = codeId;
	value.time = timePos;

	m_pDb->RemoveOne(std::make_pair(codeId, value));

}

void Bdb::CDbTable_TickHis::RemoveKey(const std::string& codeId)
{
	m_pDb->RemoveOneKey(codeId);
}

void Bdb::CDbTable_TickHis::RemoveByRange(const std::string& codeId, Long beginTime, Long endTime)
{
	ITick begin;
	begin.codeId = codeId;
	begin.time = beginTime;

	ITick end;
	end.codeId = codeId;
	end.time = endTime;

	CDb<std::string, ITick>::FieldPairRange fieldPairRange;
	fieldPairRange.beginPair = std::make_pair(codeId, begin);
	fieldPairRange.endPair = std::make_pair(codeId, end);

	m_pDb->RemoveRange(fieldPairRange);
	return;

}

void Bdb::CDbTable_TickHis::GetTicks(const std::string& codeId, const IQuery& query, ITicks& values)
{
	if (query.byReqType == 0)
	{
		GetBackWard(codeId, LLONG_MAX, query.dwSubscribeNum, values);
		return;
	}
	if (query.byReqType == 1)
	{
		GetRange(codeId, 0, LLONG_MAX, values);
		return;
	}
	if (query.byReqType == 2)
	{
		GetBackWard(codeId, query.tTime, query.dwSubscribeNum, values);
		return;

	}
	if (query.byReqType == 3)
	{
		GetForWard(codeId, query.tTime, query.dwSubscribeNum, values);
		return;

	}
	if (query.byReqType == 4)
	{
		GetRange(codeId, query.timePair.beginPos, query.timePair.endPos, values);
		return;

	}
	return;

}

void Bdb::CDbTable_TickHis::RemoveAll()
{
	m_pDb->RemoveAll();
}

bool Bdb::CDbTable_TickHis::GetLastTick(const std::string& codeId, ITick& value)
{
	IQuery query;
	query.byReqType = 2;
	query.dwSubscribeNum = 1;
	query.tTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 1;

	ITicks values;
	GetTicks(codeId, query, values);
	if (values.size() == 0) return false;

	value = values.back();
	return true;



}

void Bdb::CDbTable_TickHis::GetRange(const std::string& codeId, Long beginTime, Long endTime, ITicks& values)
{
	ITick begin;
	begin.codeId = codeId;
	begin.time = beginTime;

	ITick end;
	end.codeId = codeId;
	end.time = endTime;

	CDb<std::string, ITick>::FieldPairRange fieldPairRange;
	fieldPairRange.beginPair = std::make_pair(codeId, begin);
	fieldPairRange.endPair = std::make_pair(codeId, end);

	m_pDb->GetRange(fieldPairRange, values);


	return;

}

void Bdb::CDbTable_TickHis::GetForWard(const std::string& codeId, Long beginTime, Long count, ITicks& values)
{
	ITick value;
	value.codeId = codeId;
	value.time = beginTime;
	CDb<std::string, ITick>::FieldPair pair = std::make_pair(codeId, value);
	m_pDb->GetForWardInDup(pair, count, values);

}

void Bdb::CDbTable_TickHis::GetBackWard(const std::string& codeId, Long endTime, Long count, ITicks& values)
{
	ITick value;
	value.codeId = codeId;
	value.time = endTime;
	CDb<std::string, ITick>::FieldPair pair = std::make_pair(codeId, value);

	m_pDb->GetBackWardInDup(pair, count, values);

}

