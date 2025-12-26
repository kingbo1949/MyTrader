#pragma once

#include "Log.h"
#include "DbMapType/DbMapType_Dup.h"
#include "DbMapType/DbMapType_NoDup.h"
#include <DbJob/DbJob_AddOne.h>
#include <DbJob/DbJob_GetOne.h>
#include <DbJob/DbJob_GetAll.h>
#include <DbJob/DbJob_RemoveOne.h>
#include <DbJob/DbJob_Range_Remove.h>
#include <DbJob/DbJob_Range_Get.h>
#include <DbJob/DbJob_GetRangeKey.h>
#include <DbJob/DbJob_GetFirstOneInFile.h>
#include <DbJob/DbJob_GetFirstOneInDup.h>
#include <DbJob/DbJob_GetLastOneInDup.h>
#include <DbJob/DbJob_GetRangeKey.h>
#include <DbJob/DbJob_RemoveOneKey.h>
#include <DbJob/DbJob_GetBackWard.h>
#include <DbJob/DbJob_GetForWard.h>
#include <DbJob/DbJob_SelfCheck.h>
// #include "ToString_Q.h"


typedef int (*ComFunc) (Db *, const Dbt *, const Dbt *, size_t *locp);		// 用于排序比较的函数指针
// typedef int (*ComFunc) (Db *, const Dbt *, const Dbt *);		// 用于排序比较的函数指针
typedef u_int32_t (*PartitionFunc) (Db *, Dbt *);				// 用于分区的函数指针


namespace Bdb
{
    struct ParamForSetupDB
    {

        DbEnv*					env;
        std::string				path;
        std::string				dbName;
        ComFunc					func_key;		// key的排序规则
        ComFunc					func_value;		// value的排序规则

        // 分区所使用的字段
        u_int32_t				partNum;		// 分区的数目
        PartitionFunc			func_partition;	// 分区回调函数

        ParamForSetupDB()
        {
            env = NULL;
            path = "";
            dbName = "";
            func_key = NULL;
            func_value = NULL;
            partNum = 0;
            func_partition = NULL;
        }

    };




    template<typename KEY, typename VALUE>
    class CDb : public IceUtil::Shared
    {

    public:
        typedef IceUtil::Handle<CDb<KEY, VALUE> >DbPtr;
        typedef std::pair<KEY, VALUE>  FieldPair;
        typedef std::deque<FieldPair > FieldPairs;

        struct FieldPairRange
        {
            FieldPair	beginPair;
            FieldPair	endPair;
        };

        struct FieldKeyRange
        {
            KEY					beginKey;
            KEY					endKey;
        };

    public:
        CDb(const ParamForSetupDB& param)
            :m_paramForSetup(param), m_db(m_paramForSetup.env ,DB_CXX_NO_EXCEPTIONS)
        {
            // m_fileName = m_paramForSetup.path + m_paramForSetup.dbName;
            m_fileName = m_paramForSetup.dbName;

            if (m_paramForSetup.func_value)
            {
                m_pDBMapType = CDbMapType_Dup::Instance();
            }else
            {
                m_pDBMapType = CDbMapType_NoDup::Instance();
            }

        }
        virtual ~CDb(void)
        {
            int err = m_db.close(0);
            if (err)
            {
                printf("close database=%s failed! \n",m_fileName.c_str());
                exit(1);
                return ;
            }
            printf("database=%s is closed. \n",m_fileName.c_str());

        }

        std::string					GetDbName(){return m_fileName;};
       // DbEnv*						GetEnv(){return NULL;};
        Db*							GetDb(){return &m_db;};
        int 						Flush()
        {
            return GetDb()->sync(0);
        }
        void						Open()
        {
            Init();

            printf("Open %s \n",m_fileName.c_str());
            int err = m_db.open(NULL, m_fileName.c_str(), NULL, DB_BTREE, GetDbOpenFlag(), 0);
            if (err)
            {
				CLog::Instance()->PrintLog("", "Open %s fail!\n", m_fileName.c_str());
                exit(1);
            }
            printf("Open %s finish!\n",m_fileName.c_str());

        }

        u_int32_t					MakeFlagForAddOneJob(u_int32_t flag)
        {
            return m_pDBMapType->MakeFlagForAddOneJob(flag);
        }

        u_int32_t					MakeFlagForGetOneJob()
        {
            return m_pDBMapType->MakeFlagForGetOneJob();
        }

        u_int32_t					MakeFlagForRemoveOneJob()
        {
            return m_pDBMapType->MakeFlagForRemoveOneJob();
        }

        virtual bool				IsDup()
        {
            return m_pDBMapType->IsDup();
        }


        void						AddOne(const KEY& _first, const VALUE& _second)
        {
            typename CDbJob_AddOne<KEY, VALUE>::DbJob_AddOnePtr pJob = new CDbJob_AddOne<KEY, VALUE>(this, _first, _second);
            pJob->Execute();

        }

        void						RemoveOne(const FieldPair& inPair )
        {
            typename CDbJob_RemoveOne<KEY, VALUE>::DbJob_RemoveOnePtr pJob = new CDbJob_RemoveOne<KEY, VALUE>(this, inPair);
            pJob->Execute();

        }

        void						RemoveOneKey(const KEY& key )
        {
            typename CDbJob_RemoveOneKey<KEY, VALUE>::DbJob_RemoveOneKeyPtr pJob = new CDbJob_RemoveOneKey<KEY, VALUE>(this, key);
            pJob->Execute();

        }

        bool						GetOne(const KEY& _key, VALUE& _value)
        {
            typename CDbJob_GetOne<KEY, VALUE>::DbJob_GetOnePtr pJob = new CDbJob_GetOne<KEY, VALUE>(this, _key, _value);
            return pJob->Execute() && pJob->GetResult();


        }

        FieldPairs					GetAll()
        {
            typename  CDbJob_GetAll<KEY, VALUE>::DbJob_GetAllPtr pJob = new CDbJob_GetAll<KEY, VALUE>(this);
            pJob->Execute();

            return pJob->GetResult();

        }

        void						RemoveAll()
        {
            u_int32_t count = 0;
            m_db.truncate(NULL, &count, 0);
            return ;
        }

        void					GetRangeFieldPairs(const FieldPairRange& range, FieldPairs& result)
        {
            typename  CDbJob_Range_Get_FieldPairs<KEY, VALUE>::DbJob_Range_Get_FieldPairsPtr pJob = new CDbJob_Range_Get_FieldPairs<KEY, VALUE>(this, range,result);
            pJob->Execute();
        }

        void					GetRange(const FieldPairRange& range, std::vector<VALUE>& datas)
        {
            typename CDbJob_Range_Get_Values<KEY, VALUE>::DbJob_Range_Get_ValuesPtr pJob = new CDbJob_Range_Get_Values<KEY, VALUE>(this, range, datas);
            pJob->Execute();

        }

        void						RemoveRange(const FieldPairRange& range)
        {
            typename CDbJob_Range_Remove<KEY, VALUE>::DbJob_Range_RemovePtr pJob = new CDbJob_Range_Remove<KEY, VALUE>(this, range);
            pJob->Execute();

        }

        void					GetForWardInDup(const FieldPair& pair, Long count, FieldPairs& datas)
        {
            bool forWholeFile = false;
            typename CDbJob_GetForWard<KEY, VALUE>::DbJob_GetForWardPtr pJob = new CDbJob_GetForWard<KEY, VALUE>(this, pair, count, forWholeFile);
            pJob->Execute();

            pJob->GetFieldPairs(datas);

        }
        void					GetForWardInDup(const FieldPair& pair, Long count, std::vector<VALUE>& datas)
        {
            bool forWholeFile = false;
            typename CDbJob_GetForWard<KEY, VALUE>::DbJob_GetForWardPtr pJob = new CDbJob_GetForWard<KEY, VALUE>(this, pair, count, forWholeFile);
            pJob->Execute();

            pJob->GetResult(datas);

        }
        void					GetForWardInFile(const FieldPair& pair, Long count, FieldPairs& datas)
        {
            bool forWholeFile = true;
            typename CDbJob_GetForWard<KEY, VALUE>::DbJob_GetForWardPtr pJob = new CDbJob_GetForWard<KEY, VALUE>(this, pair, count, forWholeFile);
            pJob->Execute();

            pJob->GetFieldPairs(datas);

        }

        void					GetBackWardInDup(const FieldPair& pair, Long count, std::vector<VALUE>& datas)
        {
            // printf("GetBackWardInDup 1 \n");
            VALUE lastValue;
            bool success = GetLastOneInDup(pair.first, lastValue);
            if (!success)
            {

                return ;
            }
            // printf("GetBackWardInDup 2 \n");
            FieldPair lastPair = std::make_pair(pair.first, lastValue);

            typename CDbJob_GetBackWard<KEY, VALUE>::DbJob_GetBackWardPtr pJob = new CDbJob_GetBackWard<KEY, VALUE>(this, pair, lastPair, count);
            pJob->Execute();

            // printf("GetBackWardInDup 3 \n");

            pJob->GetResult(datas);

        }

        std::vector<KEY>					GetRangeKey(const FieldKeyRange& range)
        {
            typename CDbJob_GetRangeKey<KEY, VALUE>::DbJob_GetRangeKeyPtr pJob = new CDbJob_GetRangeKey<KEY, VALUE>(this, range);
            pJob->Execute();

            return pJob->GetResult();

        }


        bool						GetFirstOneInDup(const KEY& key, VALUE& value)
        {
            typename CDbJob_GetFirstOneInDup<KEY, VALUE>::DbJob_GetFirstOneInDupPtr pJob = new CDbJob_GetFirstOneInDup<KEY, VALUE>(this, key);
            pJob->Execute();

            return pJob->GetResult(value);

        }

        bool						GetFirstOneInFile(KEY& key, VALUE& value)
        {
            typename CDbJob_GetFirstOneInFile<KEY, VALUE>::DbJob_GetFirstOneInFilePtr pJob = new CDbJob_GetFirstOneInFile<KEY, VALUE>(this);
            pJob->Execute();

            return pJob->GetResult(key, value);

        }

        bool						GetLastOneInDup(const KEY& key, VALUE& value)
        {
            typename CDbJob_GetLastOneInDup<KEY, VALUE>::DbJob_GetLastOneInDupPtr pJob = new CDbJob_GetLastOneInDup<KEY, VALUE>(this, key);
            pJob->Execute();

            return pJob->GetResult(value);

        }

        bool						SelfCheck(const std::string& tableName)
        {
            Ice::Long beginTimeMilliSec = Time::now().toMilliSeconds();

            typename CDbJob_SelfCheck<KEY, VALUE>::DbJob_SelfCheckPtr pJob = new CDbJob_SelfCheck<KEY, VALUE>(this, tableName);
			CLog::Instance()->PrintLog(pJob->GetLogFileName(true), "%s begin... \n", tableName.c_str());
            bool success = pJob->Execute();
            typename CDbJob_SelfCheck<KEY, VALUE>::SelfCheckResult result = pJob->GetResult();

            Ice::Long endTimeMilliSec = Time::now().toMilliSeconds();

            // 开始打印
            if (!success)
            {
				CLog::Instance()->PrintLog(pJob->GetLogFileName(false), "%s selfcheck over with database err = %d \n", tableName.c_str(), result.errCode);
                return false;
            }
            if ( result.keyValid)
            {
                //ToString_QPtr pToString = new CToString_Q();
                //string str = pToString->ToString(result.key);

                //CGlobal::Instance()->PrintLog("selfcheckErr.log", "%s selfcheck over with buff err, %s \n", tableName.c_str(), str.c_str());
				CLog::Instance()->PrintLog(pJob->GetLogFileName(false), "%s selfcheck over with buff err \n", tableName.c_str());
                return false;
            }

			CLog::Instance()->PrintLog(pJob->GetLogFileName(true), "%s selfcheck over without err, recordNum = %d, used=%d \n", tableName.c_str(), result.recordCount, endTimeMilliSec-beginTimeMilliSec);

            return true;

        }

    protected:
        DbMapTypePtr				m_pDBMapType;
        std::string					m_fileName;			// database文件名

        ParamForSetupDB				m_paramForSetup;
        Db							m_db;				// database主对象


        void						Init()
        {
            m_db.set_flags(m_pDBMapType->MakeFlagForDBSetFlag());

            Set_bt_compare();
            Set_dup_compare();
            Set_partition();
            Set_pagesize();

        }

        u_int32_t					GetDbOpenFlag()
        {
            return DB_CREATE |	DB_THREAD;
        }

        // key的索引方式
        void				Set_bt_compare()
        {
            if (m_paramForSetup.func_key)
            {
                m_db.set_bt_compare(m_paramForSetup.func_key);
            }
        }

        // value的索引方式
        void				Set_dup_compare()
        {
            if (m_paramForSetup.func_value)
            {
                m_db.set_dup_compare(m_paramForSetup.func_value);
            }
        }

        // 文件分区方法
        void				Set_partition()
        {
            if (m_paramForSetup.partNum == 0 || !m_paramForSetup.func_partition) return ;

            m_db.set_partition(m_paramForSetup.partNum, NULL, m_paramForSetup.func_partition);
        };

        // 分页方法
        void				Set_pagesize(){m_db.set_pagesize(4 * 1024);};


    };



}
using namespace Bdb;


