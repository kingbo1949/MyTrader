//
// Created by kingb on 2026/1/2.
//

#include "CDbTable_Atr.h"
#include "../Compare.h"
CDbTable_Atr::CDbTable_Atr(DbEnv *env, const std::string &path, const std::string &dbName)
:CDbTable(env, path, dbName)
{
    ParamForSetupDB param;
    param.env = m_env;
    param.path = m_path;
    param.dbName = m_dbName;
    param.func_key = CCompare::QKeyA;
    param.func_value = CCompare::AtrValueA;
    param.partNum = 0;
    param.func_partition = NULL;

    m_pDb = new CDb<IQKey, IAtrValue>(param);
    m_pDb->Open();
    printf("CDbTable_Atr open \n");
}

void CDbTable_Atr::AddOne(const std::string &codeId, ITimeType timeType, const IAtrValue &value)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;
    m_pDb->AddOne(key, value);

}

void CDbTable_Atr::AddSome(const std::string &codeId, ITimeType timeType, const IAtrValues &values)
{
    for (IAtrValues::const_iterator pos = values.begin(); pos != values.end(); ++pos)
    {
        AddOne(codeId, timeType, *pos);
    }

}

bool CDbTable_Atr::GetOne(const std::string &codeId, ITimeType timeType, Long timePos, IAtrValue &value)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;

    value.time = timePos;

    return m_pDb->GetOne(key, value);

}

void CDbTable_Atr::RemoveOne(const std::string &codeId, ITimeType timeType, Long timePos)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;

    IAtrValue value;
    value.time = timePos;

    m_pDb->RemoveOne(std::make_pair(key, value));

}

void CDbTable_Atr::RemoveKey(const std::string &codeId, ITimeType timeType)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;

    m_pDb->RemoveOneKey(key);

}

void CDbTable_Atr::RemoveByRange(const std::string &codeId, ITimeType timeType, Long beginTime, Long endTime)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;

    IAtrValue begin;
    begin.time = beginTime;

    IAtrValue end;
    end.time = endTime;

    CDb<IQKey, IAtrValue>::FieldPairRange fieldPairRange;
    fieldPairRange.beginPair = std::make_pair(key, begin);
    fieldPairRange.endPair = std::make_pair(key, end);

    m_pDb->RemoveRange(fieldPairRange);
    return;

}

void CDbTable_Atr::RemoveAll()
{
    m_pDb->RemoveAll();
}

void CDbTable_Atr::GetValues(const std::string &codeId, ITimeType timeType, const IQuery &query, IAtrValues &values)
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

void CDbTable_Atr::GetRange(const std::string &codeId, ITimeType timeType, Long beginTime, Long endTime, IAtrValues &values)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;

    IAtrValue begin;
    begin.time = beginTime;

    IAtrValue end;
    end.time = endTime;

    CDb<IQKey, IAtrValue>::FieldPairRange fieldPairRange;
    fieldPairRange.beginPair = std::make_pair(key, begin);
    fieldPairRange.endPair = std::make_pair(key, end);

    m_pDb->GetRange(fieldPairRange, values);

    return;

}

void CDbTable_Atr::GetForWard(const std::string &codeId, ITimeType timeType, Long beginTime, Long count, IAtrValues &values)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;

    IAtrValue value;
    value.time = beginTime;
    CDb<IQKey, IAtrValue>::FieldPair pair = std::make_pair(key, value);
    m_pDb->GetForWardInDup(pair, count, values);

}

void CDbTable_Atr::GetBackWard(const std::string &codeId, ITimeType timeType, Long endTime, Long count, IAtrValues &values)
{
    IQKey key;
    key.codeId = codeId;
    key.timeType = timeType;

    IAtrValue value;
    value.time = endTime;
    CDb<IQKey, IAtrValue>::FieldPair pair = std::make_pair(key, value);

    m_pDb->GetBackWardInDup(pair, count, values);

}

