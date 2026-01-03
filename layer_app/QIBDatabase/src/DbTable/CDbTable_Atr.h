//
// Created by kingb on 2026/1/2.
//
#pragma once

#include "../GlobalDefine.h"
namespace Bdb
{
    class CDbTable_Atr : public CDbTable
    {
    public:
        CDbTable_Atr(DbEnv* env, const std::string& path, const std::string& dbName);
        virtual ~CDbTable_Atr(void) { ; };

        virtual	int				Flush()
        {
            return m_pDb->Flush();
        }
        virtual	bool	SelfCheck()
        {
            return m_pDb->SelfCheck(m_dbName);
        }

        void		AddOne(const std::string& codeId, ITimeType timeType, const IAtrValue& value);
        void		AddSome(const std::string& codeId, ITimeType timeType, const IAtrValues& values);
        bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IAtrValue& value);

        void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
        void		RemoveKey(const std::string& codeId, ITimeType timeType);
        void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
        void		RemoveAll();

        void		GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IAtrValues& values);





    protected:
        CDb<IQKey, IAtrValue>::DbPtr	m_pDb;

        // 取值范围[beginDayTime,endDayTime)
        void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IAtrValues& values);
        void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IAtrValues& values);
        void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IAtrValues& values);
    };
    typedef IceUtil::Handle<CDbTable_Atr> DbTable_AtrPtr;
} // Bdb

